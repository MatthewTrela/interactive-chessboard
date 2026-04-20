#include "io_expander.h"

#include "global.h"
#include "led.h"
#include "task/task.h"

bool expanderOnline[4] = {false, false, false, false};

// ========== MAPPING TABLES ==========
const InputRef switchGrid[8][8] = {{{1, 1}, {1, 5}, {1, 9}, {1, 13}, {2, 16}, {2, 12}, {2, 8}, {2, 4}},
                                   {{1, 2}, {1, 6}, {1, 10}, {1, 14}, {2, 15}, {2, 11}, {2, 7}, {2, 3}},
                                   {{1, 3}, {1, 7}, {1, 11}, {1, 15}, {2, 14}, {2, 10}, {2, 6}, {2, 2}},
                                   {{1, 4}, {1, 8}, {1, 12}, {1, 16}, {2, 13}, {2, 9}, {2, 5}, {2, 1}},
                                   {{4, 1}, {4, 5}, {4, 9}, {4, 13}, {3, 16}, {3, 12}, {3, 8}, {3, 4}},
                                   {{4, 2}, {4, 6}, {4, 10}, {4, 14}, {3, 15}, {3, 11}, {3, 7}, {3, 3}},
                                   {{4, 3}, {4, 7}, {4, 11}, {4, 15}, {3, 14}, {3, 10}, {3, 6}, {3, 2}},
                                   {{4, 4}, {4, 8}, {4, 12}, {4, 16}, {3, 13}, {3, 9}, {3, 5}, {3, 1}}};

bool getRowColFromExpanderPin(uint8_t expander, uint8_t pin, uint8_t& row, uint8_t& col) {
    for (uint8_t r = 0; r < 8; r++) {
        for (uint8_t c = 0; c < 8; c++) {
            if (switchGrid[r][c].exp1 - 1 == expander) {
                uint8_t sw = switchGrid[r][c].pin1;
                uint8_t bitIdx = swToIdx[sw];
                if (bitIdx == pin) {
                    row = r;
                    col = c;
                    return true;
                }
            }
        }
    }
    return false;
}

void enableHAEN_AllChips() {
    // Before HAEN, all chips respond to addr 0.
    // Write IOCON with HAEN=1 (bit 3) directly.
    // IOCON address in BANK=0 mode is 0x0A
    // Value: HAEN (0x08) | SEQOP disabled = 0x28 is typical, but just HAEN:
    digitalWrite(SPI_CS_MCP, LOW);
    SPI.transfer(0x40);  // Opcode: write, addr=0b000, R/W=0
    SPI.transfer(0x0A);  // IOCON register
    SPI.transfer(0x08);  // HAEN bit
    digitalWrite(SPI_CS_MCP, HIGH);
}

void initExpanders(bool polling) {
    // pinMode(SPI_CS_MCP, OUTPUT);
    // digitalWrite(SPI_CS_MCP, HIGH);

    // // Broadcast HAEN to ALL chips while they're all addr 0
    // enableHAEN_AllChips();
    // delay(2);
    MCP23S17 bootstrap(SPI_CS_MCP, 0, &SPI);
    if (bootstrap.begin()) {
        bootstrap.enableHardwareAddress();
        delay(2);
        Serial.println("HAEN bootstrap done via addr 0");
    }

    bool anyOnline = false;

    for (int i = 0; i < 4; i++) {
        expanderOnline[i] = false;

        if (!expanders[i]->begin()) {
            Serial.printf("Failed to initialize Expander %d\n", i);
            continue;
        }

        expanders[i]->enableHardwareAddress();
        expanders[i]->pinMode16(0xFFFF);
        expanders[i]->setPullup16(0xFFFF);

        if (!polling) {
            expanders[i]->mirrorInterrupts(true);
            expanders[i]->enableInterrupt16(0xFFFF, CHANGE);
            expanders[i]->setInterruptPolarity(2);  // active-low
        }

        expanderState.expander[i] = expanders[i]->read16();
        expanderOnline[i] = true;
        anyOnline = true;

        Serial.printf("Expander %d addr %d init OK, val=0x%04X\n", i, expanders[i]->getAddress(),
                      expanderState.expander[i]);
    }

    if (!polling && anyOnline) {
        pinMode(MCP_INT_PIN, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(MCP_INT_PIN), onExpanderInterrupt, FALLING);
    }
}

// ISR
void IRAM_ATTR onExpanderInterrupt() {
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    if (GameLoopTaskHandle != nullptr) {
        vTaskNotifyGiveFromISR(GameLoopTaskHandle, &higherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

uint64_t checkAllExpandersForInterrupt() {
    uint64_t newOccupancy = 0;

    for (int i = 0; i < 4; i++) {
        if (!expanderOnline[i]) continue;

        uint16_t intf = expanders[i]->getInterruptFlagRegister();
        uint16_t currentValue = expanderState.expander[i];

        if (intf != 0) {
            uint16_t newValue = readWithDebounce(expanders[i]);
            if (newValue != expanderState.expander[i]) {
                processStateChange(i, newValue);
                currentValue = newValue;
                lastDisplayUpdate = 0;
                Serial.printf("Expander %d changed: 0x%04X\n", i, newValue);
            }
        }

        uint64_t occupied = (~(uint64_t)currentValue) & 0xFFFF;
        newOccupancy |= (occupied << (i * 16));
    }

    return newOccupancy;
}

uint16_t readWithDebounce(MCP23S17* expander) {
    uint16_t firstRead = expander->read16();
    delay(MCP_DEBOUNCE_DELAY_MS);
    uint16_t secondRead = expander->read16();

    if (firstRead == secondRead) {
        return firstRead;
    } else {
        // Retry if unstable
        delay(MCP_DEBOUNCE_DELAY_MS);
        return expander->read16();
    }
}

void processStateChange(int i, uint16_t newValue) {
    uint16_t lastVals = expanderState.expander[i];
    expanderState.expander[i] = newValue;

    for (int j = 0; j < 16; j++) {
        uint16_t mask = 1 << j;

        bool isPressed = !(newValue & mask);
        bool wasPressed = !(lastVals & mask);

        if (isPressed != wasPressed) {
            setLEDFromInput(i, j, isPressed);

            if (isPressed) {
                uint8_t row, col;
                if (getRowColFromExpanderPin(i, j, row, col)) {
                    Serial.printf("Pressed: E%d, Pin %d → row=%d, col=%d\n", i, j, row, col);
                } else {
                    Serial.printf("Pressed: E%d, Pin %d\n", i, j);
                }
            } else {
                uint8_t row, col;
                if (getRowColFromExpanderPin(i, j, row, col)) {
                    Serial.printf("Released: E%d, Pin %d → row=%d, col=%d\n", i, j, row, col);
                } else {
                    Serial.printf("Released: E%d, Pin %d\n", i, j);
                }
            }
        }
    }
}

void poll() {
    for (int i = 0; i < 4; i++) {
        if (!expanderOnline[i]) continue;

        uint16_t newValue = expanders[i]->read16();

        if (newValue != expanderState.expander[i]) {
            processStateChange(i, newValue);
        }
    }
}

// bitmap functions
// ========== BITMAP FUNCTIONS ==========
uint64_t readBoardBitmap() {
    uint64_t bitmap = 0;

    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 8; col++) {
            uint8_t expander = switchGrid[row][col].exp1 - 1;  // 0-3
            uint8_t sw = switchGrid[row][col].pin1;            // 1-16

            if (expander >= 4 || sw < 1 || sw > 16) continue;

            uint8_t bitIdx = swToIdx[sw];
            if (bitIdx == 0xFF) continue;

            uint16_t regValue = expanders[expander]->read16();
            bool hasPiece = !(regValue & (1 << bitIdx));

            if (hasPiece) {
                uint8_t sq = row * 8 + col;
                bitmap |= (1ULL << sq);
            }
        }
    }

    return bitmap;
}

bool getSquareOccupied(uint8_t row, uint8_t col) {
    if (row >= 8 || col >= 8) return false;

    uint8_t expander = switchGrid[row][col].exp1 - 1;
    uint8_t sw = switchGrid[row][col].pin1;

    if (expander >= 4 || sw < 1 || sw > 16) return false;

    uint8_t bitIdx = swToIdx[sw];
    if (bitIdx == 0xFF) return false;

    uint16_t regValue = expanders[expander]->read16();
    return !(regValue & (1 << bitIdx));
}

void printBoardState() {
    uint64_t board = readBoardBitmap();

    Serial.printf("Board bitmap: 0x%016llX\n", board);
    Serial.println("  0 1 2 3 4 5 6 7");

    for (int row = 0; row < 8; row++) {
        Serial.printf("%d ", row);
        for (int col = 0; col < 8; col++) {
            bool occupied = (board >> (row * 8 + col)) & 1ULL;
            Serial.print(occupied ? "X " : ". ");
        }
        Serial.println();
    }
    Serial.println();
}