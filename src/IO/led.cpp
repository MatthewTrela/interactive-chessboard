#include "led.h"

#include <Adafruit_NeoPixel.h>

#include "global.h"

// ========== LED BUFFERS ==========
static uint32_t ledBuffer[8][8];    // Desired states
static uint32_t ledPhysical[8][8];  // Current physical states
static bool ledDirty[8][8];         // Which LEDs need updating

// ========== MAPPING TABLE ==========
// [expander][pin] = {row, col}
// Initialize all to invalid values (0xFF)
static struct {
    uint8_t row;
    uint8_t col;
    bool valid;
} pinMapping[4][16];

struct InputRef {
    uint8_t exp1;
    uint8_t pin1;
};

const uint32_t base_color = 0x00FF00;

static const uint8_t swToIdx[17] = {
    0xFF,
    13,  // SW1  = GPA5
    12,  // SW2  = GPA4
    11,  // SW3  = GPA3
    10,  // SW4  = GPA2
    15,  // SW5  = GPA7
    14,  // SW6  = GPA6
    8,   // SW7  = GPA0
    9,   // SW8  = GPA1
    0,   // SW9  = GPB0
    1,   // SW10 = GPB1
    5,   // SW11 = GPB5
    7,   // SW12 = GPB7
    2,   // SW13 = GPB2
    3,   // SW14 = GPB3
    4,   // SW15 = GPB4
    6    // SW16 = GPB6
};

const InputRef switchGrid[8][8] = {{{1, 1}, {1, 5}, {1, 9}, {1, 13}, {2, 16}, {2, 12}, {2, 8}, {2, 4}},
                                   {{1, 2}, {1, 6}, {1, 10}, {1, 14}, {2, 15}, {2, 11}, {2, 7}, {2, 3}},
                                   {{1, 3}, {1, 7}, {1, 11}, {1, 15}, {2, 14}, {2, 10}, {2, 6}, {2, 2}},
                                   {{1, 4}, {1, 8}, {1, 12}, {1, 16}, {2, 13}, {2, 9}, {2, 5}, {2, 1}},
                                   {{4, 1}, {4, 5}, {4, 9}, {4, 13}, {3, 16}, {3, 12}, {3, 8}, {3, 4}},
                                   {{4, 2}, {4, 6}, {4, 10}, {4, 14}, {3, 15}, {3, 11}, {3, 7}, {3, 3}},
                                   {{4, 3}, {4, 7}, {4, 11}, {4, 15}, {3, 14}, {3, 10}, {3, 6}, {3, 2}},
                                   {{4, 4}, {4, 8}, {4, 12}, {4, 16}, {3, 13}, {3, 9}, {3, 5}, {3, 1}}};

// ========== FLUSH THROTTLING ==========
static unsigned long lastFlushTime = 0;

// ========== HELPER FUNCTIONS ==========
uint8_t getLEDIndex(uint8_t row, uint8_t col) {
    if ((row % 2) == 0) {
        return row * 8 + col;  // even row: L->R
    } else {
        return row * 8 + (7 - col);  // odd row: R->L
    }
}

bool getSwitchStateFromGrid(uint8_t row, uint8_t col) {
    if (row >= 8 || col >= 8) return false;

    uint8_t expander = switchGrid[row][col].exp1;  // 1-based
    uint8_t sw = switchGrid[row][col].pin1;        // 1..16

    if (expander < 1 || expander > 4 || sw < 1 || sw > 16) return false;

    uint8_t idx = swToIdx[sw];  // bit position (0-15)
    if (idx == 0xFF) return false;

    return !(mcpValues[expander - 1] & (1 << idx));  // active-low
}

// ========== INITIALIZATION ==========
void initLEDs() {
    Serial.println("Initializing LED strip...");

    strip->begin();
    strip->setBrightness(LED_BRIGHTNESS);
    strip->show();  // Initialize all pixels to off

    clearAllLEDs();

    // Load mapping
    loadCustomMapping();

    Serial.println("LED strip initialized");

    testAllLEDs();
}

// ========== LED CONTROL (BUFFERED) ==========
// Note: Ensure your global ledBuffer is updated to: CRGB ledBuffer[8][8];

void setLED(uint8_t row, uint8_t col, uint32_t color) {
    if (row >= 8 || col >= 8) {
        Serial.print("ERROR: Invalid LED position (row=");
        Serial.print(row);
        Serial.print(", col=");
        Serial.println(col);
        return;
    }
    if (ledBuffer[row][col] != color) {
        ledBuffer[row][col] = color;
        ledDirty[row][col] = true;
    }
}

void setLEDFromInput(uint8_t expander, uint8_t pin, bool state) {
    if (expander >= 4 || pin >= 16) {
        Serial.print("ERROR: Invalid input (expander=");
        Serial.print(expander);
        Serial.print(", pin=");
        Serial.println(pin);
        return;
    }

    // Check if this pin has a mapping
    if (!pinMapping[expander][pin].valid) {
        Serial.print("WARNING: No LED mapping for Expander ");
        Serial.print(expander);
        Serial.print(", Pin ");
        Serial.println(pin);
        return;
    }

    // Get mapped LED position
    uint8_t row = pinMapping[expander][pin].row;
    uint8_t col = pinMapping[expander][pin].col;

    // Set LED state
    if (state) {
        setLED(row, col, base_color);
    } else {
        setLED(row, col, 0x000000);
    }
}

void syncLEDsFromInputState() {
    Serial.println("Syncing LEDs from current input state...");

    for (int i = 0; i < NUM_EXPANDERS; i++) {
        uint16_t value = mcpValues[i];

        for (int pin = 0; pin < 16; pin++) {
            if (pinMapping[i][pin].valid) {
                bool isPressed = !(value & (1 << pin));
                setLEDFromInput(i, pin, isPressed);
            }
        }
    }

    flushLEDBuffer();
    Serial.println("LED sync complete");
}

// ========== BUFFER FLUSHING ==========
void flushLEDBuffer() {
    unsigned long now = millis();

    // throttle update
    if (now - lastFlushTime < LED_MIN_FLUSH_MS) {
        return;
    }

    bool anyUpdate = false;

    // scan for dirty flag
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (ledDirty[row][col]) {
                uint8_t index = getLEDIndex(row, col);
                uint32_t color = ledBuffer[row][col];

                // Update physical strip
                strip->setPixelColor(index, color);

                // Update physical state buffer
                ledPhysical[row][col] = color;

                // Clear dirty flag
                ledDirty[row][col] = false;
                anyUpdate = true;
            }
        }
    }

    if (anyUpdate) {
        strip->show();
        lastFlushTime = millis();
        ;

        // Optional debug: print which LEDs updated
        // Serial.println("LED buffer flushed");
    }
}

// ========== BULK OPERATIONS ==========
void clearAllLEDs() {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            setLED(row, col, 0x000000);
        }
    }
}

void setAllLEDs(uint32_t color) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            setLED(row, col, color);
        }
    }
}

// ========== MAPPING CONFIGURATION ==========
void setLEDMapping(uint8_t expander, uint8_t pin, uint8_t row, uint8_t col) {
    // Validate inputs
    if (expander >= 4 || pin >= 16) {
        Serial.print("ERROR: Invalid mapping (expander=");
        Serial.print(expander);
        Serial.print(", pin=");
        Serial.println(pin);
        return;
    }

    if (row >= 8 || col >= 8) {
        Serial.print("ERROR: Invalid LED position (row=");
        Serial.print(row);
        Serial.print(", col=");
        Serial.println(col);
        return;
    }

    // Store mapping
    pinMapping[expander][pin].row = row;
    pinMapping[expander][pin].col = col;
    pinMapping[expander][pin].valid = true;

    Serial.print("Mapped Expander ");
    Serial.print(expander);
    Serial.print(", Pin ");
    Serial.print(pin);
    Serial.print(" → LED (");
    Serial.print(row);
    Serial.print(",");
    Serial.print(col);
    Serial.println(")");
}

void loadCustomMapping() {
    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 8; col++) {
            uint8_t exp1 = switchGrid[row][col].exp1;  // 1..4
            uint8_t sw = switchGrid[row][col].pin1;    // SW number 1..16

            if (exp1 < 1 || exp1 > 4 || sw < 1 || sw > 16) continue;

            uint8_t idx = swToIdx[sw];
            if (idx == 0xFF) continue;
            setLEDMapping(exp1 - 1, idx, row, col);
        }
    }
}

// ========== TEST FUNCTIONS ==========
void testAllLEDs() {
    Serial.println("Testing all LEDs...");

    // Test pattern: light each LED one by one
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            clearAllLEDs();
            setLED(row, col, base_color);
            flushLEDBuffer();
            delay(15);
        }
    }

    // Test pattern: all on
    setAllLEDs(base_color);
    flushLEDBuffer();
    delay(2000);

    // Test pattern: all off
    clearAllLEDs();
    flushLEDBuffer();

    Serial.println("LED test complete");
}

// Board translation methods

uint64_t readBoardBitmap() {
    uint64_t bitmap = 0;

    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 8; col++) {
            uint8_t expander = switchGrid[row][col].exp1 - 1;  // 0-3
            uint8_t sw = switchGrid[row][col].pin1;            // 1-16

            if (expander >= 4 || sw < 1 || sw > 16) continue;

            uint8_t bitIdx = swToIdx[sw];
            if (bitIdx == 0xFF) continue;

            // Read the expander register
            uint16_t regValue = expanders[expander]->read16();

            // Active-low: 0 = no piece, 1 = piece present
            bool hasPiece = !(regValue & (1 << bitIdx));

            if (hasPiece) {
                uint8_t sq = row * 8 + col;  // chess bitboard format
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