#include "led.h"

#include <Adafruit_NeoPixel.h>

#include "global.h"

// ========== LED BUFFERS ==========
static bool ledBuffer[8][8];
static bool ledPhysical[8][8];
static bool ledDirty[8][8];

// ========== MAPPING TABLE ==========
static struct {
    uint8_t row;
    uint8_t col;
    bool valid;
} pinMapping[4][16];

// ========== FLUSH THROTTLING ==========
static unsigned long lastFlushTime = 0;

// ========== HELPER FUNCTIONS ==========
uint8_t getLEDIndex(uint8_t row, uint8_t col) { return (row * 8) + col; }

// ========== INITIALIZATION ==========
void initLEDs() {
    Serial.println("Initializing LED strip...");

    strip->begin();
    strip->setBrightness(LED_BRIGHTNESS);
    strip->show();

    clearAllLEDs();

    for (int exp = 0; exp < 4; exp++) {
        for (int pin = 0; pin < 16; pin++) {
            pinMapping[exp][pin].valid = false;
        }
    }

    loadDefaultMapping();

    Serial.println("LED strip initialized");

    testAllLEDs();
}

// ========== LED CONTROL ==========
void setLED(uint8_t row, uint8_t col, bool state) {
    if (row >= 8 || col >= 8) return;

    if (ledBuffer[row][col] != state) {
        ledBuffer[row][col] = state;
        ledDirty[row][col] = true;
    }
}

void setLEDFromInput(uint8_t expander, uint8_t pin, bool state) {
    if (expander >= 4 || pin >= 16) return;

    if (!pinMapping[expander][pin].valid) return;

    uint8_t row = pinMapping[expander][pin].row;
    uint8_t col = pinMapping[expander][pin].col;
    setLED(row, col, state);
}

// ========== BUFFER FLUSHING ==========
void flushLEDBuffer() {
    unsigned long now = millis();

    if (now - lastFlushTime < LED_MIN_FLUSH_MS) {
        return;
    }

    bool anyUpdate = false;

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (ledDirty[row][col]) {
                uint8_t index = getLEDIndex(row, col);
                uint32_t color = ledBuffer[row][col] ? 0xFFFFFF : 0x000000;
                strip->setPixelColor(index, color);
                ledPhysical[row][col] = ledBuffer[row][col];
                ledDirty[row][col] = false;
                anyUpdate = true;
            }
        }
    }

    if (anyUpdate) {
        strip->show();
        lastFlushTime = millis();
    }
}

// ========== BULK OPERATIONS ==========
void clearAllLEDs() {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            setLED(row, col, LED_OFF);
        }
    }
}

void setAllLEDs(bool state) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            setLED(row, col, state);
        }
    }
}

// ========== MAPPING CONFIGURATION ==========
void setLEDMapping(uint8_t expander, uint8_t pin, uint8_t row, uint8_t col) {
    if (expander >= 4 || pin >= 16 || row >= 8 || col >= 8) return;

    pinMapping[expander][pin].row = row;
    pinMapping[expander][pin].col = col;
    pinMapping[expander][pin].valid = true;
}

void clearLEDMapping(uint8_t expander, uint8_t pin) {
    if (expander >= 4 || pin >= 16) return;
    pinMapping[expander][pin].valid = false;
}

void loadDefaultMapping() {
    Serial.println("Loading default LED mapping...");

    for (int exp = 0; exp < 4; exp++) {
        for (int pin = 0; pin < 16; pin++) {
            int row = (exp * 2) + (pin / 8);
            int col = pin % 8;
            setLEDMapping(exp, pin, row, col);
        }
    }
}

void loadCustomMapping() {
    Serial.println("Loading custom LED mapping - PLACEHOLDER");
    // User will populate via calibration
}

// ========== MAPPING QUERY FUNCTIONS ==========
bool getLEDMapping(uint8_t expander, uint8_t pin, uint8_t& row, uint8_t& col) {
    if (expander >= 4 || pin >= 16) return false;
    if (!pinMapping[expander][pin].valid) return false;
    row = pinMapping[expander][pin].row;
    col = pinMapping[expander][pin].col;
    return true;
}

int getLEDIndexFromRowCol(uint8_t row, uint8_t col) { return getLEDIndex(row, col); }

int countMappedPins() {
    int count = 0;
    for (int exp = 0; exp < 4; exp++) {
        for (int pin = 0; pin < 16; pin++) {
            if (pinMapping[exp][pin].valid) count++;
        }
    }
    return count;
}

bool isInputMapped(uint8_t expander, uint8_t pin) {
    if (expander >= 4 || pin >= 16) return false;
    return pinMapping[expander][pin].valid;
}

// ========== TEST FUNCTIONS ==========
void testAllLEDs() {
    Serial.println("Testing all LEDs...");

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            clearAllLEDs();
            setLED(row, col, LED_ON);
            flushLEDBuffer();
            delay(100);
        }
    }

    setAllLEDs(LED_ON);
    flushLEDBuffer();
    delay(200);

    clearAllLEDs();
    flushLEDBuffer();

    Serial.println("LED test complete");
}