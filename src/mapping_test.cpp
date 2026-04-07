#include "mapping_test.h"

#include <EEPROM.h>

#include "global.h"
#include "led.h"
#include "oled.h"

// EEPROM storage
#define EEPROM_MAGIC_ADDR 0
#define EEPROM_MAPPING_START 10
#define EEPROM_MAGIC_VALUE 0xCA1D
#define EEPROM_SIZE 512

// ========== STATE ==========
static uint8_t targetExpander = 0;
static uint8_t targetPin = 0;
static uint8_t cursorRow = 0;
static uint8_t cursorCol = 0;

static uint16_t lastInputStates[4] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static unsigned long lastPressTime = 0;

static char serialBuffer[64];
static int serialBufferPos = 0;

static bool mappingModeActive = false;

// ========== HELPERS ==========
static int getMappingIndex(uint8_t expander, uint8_t pin) { return (expander * 16) + pin; }

static void showHelp() {
    Serial.println("\n========== MAPPING MODE ==========");
    Serial.println("h          - Show this help");
    Serial.println("q          - Exit mapping mode");
    Serial.println("");
    Serial.println("e <0-3>    - Set target EXPANDER");
    Serial.println("p <0-15>   - Set target PIN");
    Serial.println("r <0-7>    - Set cursor ROW");
    Serial.println("c <0-7>    - Set cursor COL");
    Serial.println("");
    Serial.println("s          - SAVE mapping");
    Serial.println("x          - CLEAR current mapping");
    Serial.println("n          - NEXT unmapped input");
    Serial.println("");
    Serial.println("d          - Display all mappings");
    Serial.println("v          - Visual board (X=mapped, P=pressed)");
    Serial.println("u          - List unmapped inputs");
    Serial.println("a          - Show all input states");
    Serial.println("");
    Serial.println("W          - Save to EEPROM");
    Serial.println("L          - Load from EEPROM");
    Serial.println("X          - Reset ALL mappings");
    Serial.println("==================================\n");
}

static void showCurrentState() {
    Serial.print("\nTarget: E");
    Serial.print(targetExpander);
    Serial.print("P");
    Serial.print(targetPin);
    Serial.print(" -> LED [");
    Serial.print(cursorRow);
    Serial.print(",");
    Serial.print(cursorCol);
    Serial.print("]  (index ");
    Serial.print(getLEDIndexFromRowCol(cursorRow, cursorCol));
    Serial.println(")");

    if (isInputMapped(targetExpander, targetPin)) {
        uint8_t r, c;
        getLEDMapping(targetExpander, targetPin, r, c);
        Serial.print("  Already mapped to [");
        Serial.print(r);
        Serial.print(",");
        Serial.print(c);
        Serial.println("]");
    }

    Serial.print("Progress: ");
    Serial.print(countMappedPins());
    Serial.print("/");
    Serial.print(TOTAL_INPUTS);
    Serial.println(" mapped\n");
}

static void showAllMappings() {
    Serial.println("\n========== ALL MAPPINGS ==========");
    Serial.println(" E  P  ->  R  C");
    Serial.println("--------------------");

    for (int exp = 0; exp < 4; exp++) {
        for (int pin = 0; pin < 16; pin++) {
            uint8_t row, col;
            if (getLEDMapping(exp, pin, row, col)) {
                Serial.print(" ");
                Serial.print(exp);
                Serial.print("  ");
                Serial.print(pin);
                Serial.print("  ->  ");
                Serial.print(row);
                Serial.print("  ");
                Serial.print(col);
                Serial.print("  (idx ");
                Serial.print(getLEDIndexFromRowCol(row, col));
                Serial.println(")");
            }
        }
    }
    Serial.println("=================================\n");
}

static void showBoardVisual() {
    Serial.println("\n====== LED BOARD VISUAL ======");
    Serial.println("X=mapped, P=pressed, .=empty");
    Serial.println("    0 1 2 3 4 5 6 7  <- COL");

    for (int row = 0; row < 8; row++) {
        Serial.print("R");
        Serial.print(row);
        Serial.print(" [");

        for (int col = 0; col < 8; col++) {
            bool mapped = false;
            bool pressed = false;

            for (int exp = 0; exp < 4; exp++) {
                for (int pin = 0; pin < 16; pin++) {
                    uint8_t r, c;
                    if (getLEDMapping(exp, pin, r, c) && r == row && c == col) {
                        mapped = true;
                        if (!(mcpValues[exp] & (1 << pin))) {
                            pressed = true;
                        }
                    }
                }
            }

            if (pressed)
                Serial.print("P ");
            else if (mapped)
                Serial.print("X ");
            else
                Serial.print(". ");
        }

        Serial.print("]  ");
        Serial.println(row % 2 == 0 ? "<-" : "->");
    }
    Serial.println("===========================\n");
}

static void showInputStates() {
    Serial.println("\n========== INPUT STATES ==========");
    for (int e = 0; e < 4; e++) {
        Serial.print("E");
        Serial.print(e);
        Serial.print(": ");
        uint16_t val = mcpValues[e];
        for (int p = 15; p >= 0; p--) {
            Serial.print((val >> p) & 1);
            if (p % 4 == 0) Serial.print(" ");
        }
        Serial.print("  0x");
        Serial.println(val, HEX);
    }
    Serial.println("================================\n");
}

static void showUnmappedInputs() {
    Serial.println("\n========== UNMAPPED INPUTS ==========");
    int count = 0;
    for (int exp = 0; exp < 4; exp++) {
        for (int pin = 0; pin < 16; pin++) {
            if (!isInputMapped(exp, pin)) {
                Serial.print("E");
                Serial.print(exp);
                Serial.print("P");
                Serial.print(pin);
                Serial.print(" ");
                count++;
                if (count % 8 == 0) Serial.println();
            }
        }
    }
    if (count == 0) Serial.println("ALL INPUTS MAPPED!");
    Serial.println("\n===================================\n");
}

static void findNextUnmapped() {
    for (int i = 0; i < TOTAL_INPUTS; i++) {
        int exp = i / 16;
        int pin = i % 16;
        if (!isInputMapped(exp, pin)) {
            targetExpander = exp;
            targetPin = pin;
            cursorRow = i / 8;
            cursorCol = i % 8;
            Serial.print("Next unmapped: E");
            Serial.print(exp);
            Serial.print("P");
            Serial.println(pin);
            return;
        }
    }
    Serial.println("All inputs are mapped!");
}

static void saveCurrentMapping() {
    setLEDMapping(targetExpander, targetPin, cursorRow, cursorCol);
    Serial.print("Saved: E");
    Serial.print(targetExpander);
    Serial.print("P");
    Serial.print(targetPin);
    Serial.print(" -> [");
    Serial.print(cursorRow);
    Serial.print(",");
    Serial.print(cursorCol);
    Serial.println("]");
}

static void clearCurrentMapping() {
    Serial.print("Cleared mapping for E");
    Serial.print(targetExpander);
    Serial.print("P");
    Serial.println(targetPin);
    // Note: clearLEDMapping function needs to be added to led.cpp
}

static void saveMappingsToEEPROM() {
    Serial.println("Saving mappings to EEPROM...");

    EEPROM.put(EEPROM_MAGIC_ADDR, (uint16_t)EEPROM_MAGIC_VALUE);

    for (int exp = 0; exp < 4; exp++) {
        for (int pin = 0; pin < 16; pin++) {
            int idx = getMappingIndex(exp, pin);
            int addr = EEPROM_MAPPING_START + (idx * 3);  // 3 bytes per mapping

            uint8_t row = 0xFF, col = 0xFF, valid = 0;
            if (isInputMapped(exp, pin)) {
                getLEDMapping(exp, pin, row, col);
                valid = 1;
            }

            EEPROM.write(addr, valid);
            EEPROM.write(addr + 1, row);
            EEPROM.write(addr + 2, col);
        }
    }

    Serial.println("Mappings saved!");
}

static void loadMappingsFromEEPROM() {
    uint16_t magic;
    EEPROM.get(EEPROM_MAGIC_ADDR, magic);

    if (magic != EEPROM_MAGIC_VALUE) {
        Serial.println("No valid mapping found in EEPROM");
        return;
    }

    Serial.println("Loading mappings from EEPROM...");

    for (int exp = 0; exp < 4; exp++) {
        for (int pin = 0; pin < 16; pin++) {
            int idx = getMappingIndex(exp, pin);
            int addr = EEPROM_MAPPING_START + (idx * 3);

            uint8_t valid = EEPROM.read(addr);
            if (valid == 1) {
                uint8_t row = EEPROM.read(addr + 1);
                uint8_t col = EEPROM.read(addr + 2);
                setLEDMapping(exp, pin, row, col);
            }
        }
    }

    Serial.print("Loaded ");
    Serial.print(countMappedPins());
    Serial.println(" mappings");
}

static void resetAllMappings() {
    Serial.println("WARNING: Clearing ALL mappings!");

    for (int exp = 0; exp < 4; exp++) {
        for (int pin = 0; pin < 16; pin++) {
            // Would need clearLEDMapping function
        }
    }

    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }

    Serial.println("All mappings cleared!");
}

// ========== SERIAL COMMAND PROCESSING ==========
static void parseCommand(const char* cmd) {
    if (cmd[0] == 0) return;

    char c = cmd[0];
    int val = atoi(cmd + 1);

    switch (c) {
        case 'h':
        case 'H':
            showHelp();
            break;

        case 'q':
        case 'Q':
            mappingModeActive = false;
            Serial.println("Exiting mapping mode");
            break;

        case 'e':
        case 'E':
            if (val >= 0 && val <= 3) {
                targetExpander = val;
                Serial.print("Target expander: ");
                Serial.println(val);
            }
            break;

        case 'p':
        case 'P':
            if (val >= 0 && val <= 15) {
                targetPin = val;
                Serial.print("Target pin: ");
                Serial.println(val);
            }
            break;

        case 'r':
        case 'R':
            if (val >= 0 && val <= 7) {
                cursorRow = val;
                Serial.print("Cursor row: ");
                Serial.println(val);
            }
            break;

        case 'c':
        case 'C':
            if (val >= 0 && val <= 7) {
                cursorCol = val;
                Serial.print("Cursor col: ");
                Serial.println(val);
            }
            break;

        case 's':
        case 'S':
            saveCurrentMapping();
            break;

        case 'x':
        case 'X':
            if (cmd[1] == 0) {
                clearCurrentMapping();
            } else {
                resetAllMappings();
            }
            break;

        case 'n':
        case 'N':
            findNextUnmapped();
            break;

        case 'd':
        case 'D':
            showAllMappings();
            break;

        case 'v':
        case 'V':
            showBoardVisual();
            break;

        case 'u':
        case 'U':
            showUnmappedInputs();
            break;

        case 'a':
        case 'A':
            showInputStates();
            break;

        case 'w':
        case 'W':
            saveMappingsToEEPROM();
            break;

        case 'l':
        case 'L':
            loadMappingsFromEEPROM();
            break;

        case 'g':
        case 'G':
            if (val >= 0 && val < TOTAL_INPUTS) {
                targetExpander = val / 16;
                targetPin = val % 16;
                Serial.print("Jumped to E");
                Serial.print(targetExpander);
                Serial.print("P");
                Serial.println(targetPin);
            }
            break;

        default:
            Serial.print("Unknown: ");
            Serial.println(cmd);
    }

    showCurrentState();
}

static void processSerialCommand() {
    if (Serial.available() <= 0) return;

    char c = Serial.read();

    if (c == '\r' || c == '\n') {
        Serial.println();
        serialBuffer[serialBufferPos] = 0;
        serialBufferPos = 0;
        parseCommand(serialBuffer);
        return;
    }

    if (c == 8 || c == 127) {  // Backspace
        if (serialBufferPos > 0) {
            serialBufferPos--;
            Serial.print(" \b");
        }
        return;
    }

    if (serialBufferPos < (int)(sizeof(serialBuffer) - 1)) {
        serialBuffer[serialBufferPos++] = c;
        Serial.print(c);  // Echo
    }
}

// ========== INPUT DETECTION ==========
static void checkForInputChanges() {
    unsigned long now = millis();

    for (int e = 0; e < 4; e++) {
        uint16_t current = mcpValues[e];
        uint16_t last = lastInputStates[e];

        // Rising edge = button released
        uint16_t released = ~last & current;

        // Falling edge = button pressed
        uint16_t pressed = last & ~current;

        for (int p = 0; p < 16; p++) {
            if (pressed & (1 << p)) {
                if (now - lastPressTime > DEBOUNCE_TIME_MS) {
                    targetExpander = e;
                    targetPin = p;

                    Serial.print("\n>>> PRESSED: E");
                    Serial.print(e);
                    Serial.print("P");
                    Serial.print(p);
                    Serial.print(" (index ");
                    Serial.print(getMappingIndex(e, p));
                    Serial.println(") <<<\n");

                    if (isInputMapped(e, p)) {
                        getLEDMapping(e, p, cursorRow, cursorCol);
                        Serial.print("Already mapped to [");
                        Serial.print(cursorRow);
                        Serial.print(",");
                        Serial.print(cursorCol);
                        Serial.println("]");
                    }

                    showCurrentState();
                }
                lastPressTime = now;
            }
        }

        lastInputStates[e] = current;
    }
}

// ========== LED DISPLAY ==========
static void updateMappingLEDs() {
    // Clear all
    for (int i = 0; i < 64; i++) {
        strip->setPixelColor(i, 0);
    }

    // Show mapped LEDs
    for (int exp = 0; exp < 4; exp++) {
        for (int pin = 0; pin < 16; pin++) {
            uint8_t row, col;
            if (getLEDMapping(exp, pin, row, col)) {
                int idx = getLEDIndexFromRowCol(row, col);

                if (!(mcpValues[exp] & (1 << pin))) {
                    strip->setPixelColor(idx, 0x00FF00);  // Bright green = pressed
                } else {
                    strip->setPixelColor(idx, 0x003300);  // Dim green = mapped
                }
            }
        }
    }

    // Show cursor (current target)
    uint8_t r, c;
    if (!isInputMapped(targetExpander, targetPin) || !getLEDMapping(targetExpander, targetPin, r, c) ||
        (r != cursorRow || c != cursorCol)) {
        int idx = getLEDIndexFromRowCol(cursorRow, cursorCol);
        strip->setPixelColor(idx, 0xFFFFFF);  // White = cursor
    }

    strip->show();
}

// ========== PUBLIC FUNCTIONS ==========
void initMappingTest() {
    Serial.println("\n\n========================================");
    Serial.println("     IO EXPANDER -> LED MAPPING TEST   ");
    Serial.println("========================================");
    Serial.println();
    Serial.println("1. Press any physical switch");
    Serial.println("2. Note the E#P# output in serial");
    Serial.println("3. Set row/col with r/c commands");
    Serial.println("4. Save with 's' command");
    Serial.println();

    mappingModeActive = true;
    findNextUnmapped();
    showHelp();
    showCurrentState();

    // Update OLED
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->println("MAPPING MODE");
    display->println("");
    display->println("Press switch to");
    display->println("identify input");
    display->println("");
    display->println("Use serial for");
    display->println("calibration");
    display->display();
}

void updateMappingTest() {
    processSerialCommand();
    checkForInputChanges();
    updateMappingLEDs();
}

int countPressedInputs() {
    int count = 0;
    for (int e = 0; e < 4; e++) {
        for (int p = 0; p < 16; p++) {
            if (!(mcpValues[e] & (1 << p))) count++;
        }
    }
    return count;
}