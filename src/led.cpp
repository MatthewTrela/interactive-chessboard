#include "led.h"

#include <Adafruit_NeoPixel.h>

#include "global.h"

// ========== LED BUFFERS ==========
static bool ledBuffer[8][8];    // Desired states
static bool ledPhysical[8][8];  // Current physical states
static bool ledDirty[8][8];     // Which LEDs need updating

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
const InputRef switchGrid[8][8] = {{{1, 1}, {1, 5}, {1, 9}, {1, 13}, {2, 16}, {2, 12}, {2, 8}, {2, 4}},
                                   {{1, 2}, {1, 6}, {1, 10}, {1, 14}, {2, 15}, {2, 11}, {2, 7}, {2, 3}},
                                   {{1, 3}, {1, 7}, {1, 11}, {1, 15}, {2, 14}, {2, 10}, {2, 6}, {2, 2}},
                                   {{1, 4}, {1, 8}, {1, 12}, {1, 16}, {2, 13}, {2, 19}, {2, 5}, {2, 1}}};

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
void setLED(uint8_t row, uint8_t col, bool state) {
    if (row >= 8 || col >= 8) {
        Serial.print("ERROR: Invalid LED position (row=");
        Serial.print(row);
        Serial.print(", col=");
        Serial.print(col);
        Serial.println(")");
        return;
    }

    // update if state changed
    if (ledBuffer[row][col] != state) {
        ledBuffer[row][col] = state;
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
    setLED(row, col, state);
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
                uint32_t color = ledBuffer[row][col] ? 0xFFFFFF : 0x000000;  // White or off

                // Update physical strip
                strip->setPixelColor(index, color);

                // Update physical state buffer
                ledPhysical[row][col] = ledBuffer[row][col];

                // Clear dirty flag
                ledDirty[row][col] = false;
                anyUpdate = true;
            }
        }
    }

    if (anyUpdate) {
        strip->show();
        now = millis();
        lastFlushTime = now;

        // Optional debug: print which LEDs updated
        // Serial.println("LED buffer flushed");
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
            uint8_t exp0 = switchGrid[row][col].exp1 - 1;
            uint8_t pin0 = switchGrid[row][col].pin1 - 1;
            setLEDMapping(exp0, pin0, row, col);
        }
    }
}

void loadCustomMapping() {
    Serial.println("Loading custom LED mapping...");

    // Each expander has 16 pins (0-15)
    // Each LED is at a grid position (row 0-7, col 0-7)
    // setLEDMapping(expander, pin, row, col);

    // Expander 0 (address 0)
    setLEDMapping(0, 0, 0, 0);
    setLEDMapping(0, 1, 0, 1);
    setLEDMapping(0, 2, 0, 2);
    setLEDMapping(0, 3, 0, 3);
    setLEDMapping(0, 4, 0, 4);
    setLEDMapping(0, 5, 0, 5);
    setLEDMapping(0, 6, 0, 6);
    setLEDMapping(0, 7, 0, 7);
    setLEDMapping(0, 8, 1, 0);
    setLEDMapping(0, 9, 1, 1);
    setLEDMapping(0, 10, 1, 2);
    setLEDMapping(0, 11, 1, 3);
    setLEDMapping(0, 12, 1, 4);
    setLEDMapping(0, 13, 1, 5);
    setLEDMapping(0, 14, 1, 6);
    setLEDMapping(0, 15, 1, 7);

    // Expander 1 (address 1)
    setLEDMapping(1, 0, 2, 0);
    setLEDMapping(1, 1, 2, 1);
    setLEDMapping(1, 2, 2, 2);
    setLEDMapping(1, 3, 2, 3);
    setLEDMapping(1, 4, 2, 4);
    setLEDMapping(1, 5, 2, 5);
    setLEDMapping(1, 6, 2, 6);
    setLEDMapping(1, 7, 2, 7);
    setLEDMapping(1, 8, 3, 0);
    setLEDMapping(1, 9, 3, 1);
    setLEDMapping(1, 10, 3, 2);
    setLEDMapping(1, 11, 3, 3);
    setLEDMapping(1, 12, 3, 4);
    setLEDMapping(1, 13, 3, 5);
    setLEDMapping(1, 14, 3, 6);
    setLEDMapping(1, 15, 3, 7);

    // Expander 2 (address 2)
    setLEDMapping(2, 0, 4, 0);
    setLEDMapping(2, 1, 4, 1);
    setLEDMapping(2, 2, 4, 2);
    setLEDMapping(2, 3, 4, 3);
    setLEDMapping(2, 4, 4, 4);
    setLEDMapping(2, 5, 4, 5);
    setLEDMapping(2, 6, 4, 6);
    setLEDMapping(2, 7, 4, 7);
    setLEDMapping(2, 8, 5, 0);
    setLEDMapping(2, 9, 5, 1);
    setLEDMapping(2, 10, 5, 2);
    setLEDMapping(2, 11, 5, 3);
    setLEDMapping(2, 12, 5, 4);
    setLEDMapping(2, 13, 5, 5);
    setLEDMapping(2, 14, 5, 6);
    setLEDMapping(2, 15, 5, 7);

    // Expander 3 (address 3)
    setLEDMapping(3, 0, 6, 0);
    setLEDMapping(3, 1, 6, 1);
    setLEDMapping(3, 2, 6, 2);
    setLEDMapping(3, 3, 6, 3);
    setLEDMapping(3, 4, 6, 4);
    setLEDMapping(3, 5, 6, 5);
    setLEDMapping(3, 6, 6, 6);
    setLEDMapping(3, 7, 6, 7);
    setLEDMapping(3, 8, 7, 0);
    setLEDMapping(3, 9, 7, 1);
    setLEDMapping(3, 10, 7, 2);
    setLEDMapping(3, 11, 7, 3);
    setLEDMapping(3, 12, 7, 4);
    setLEDMapping(3, 13, 7, 5);
    setLEDMapping(3, 14, 7, 6);
    setLEDMapping(3, 15, 7, 7);
}

// ========== TEST FUNCTIONS ==========
void testAllLEDs() {
    Serial.println("Testing all LEDs...");

    // Test pattern: light each LED one by one
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            clearAllLEDs();
            setLED(row, col, LED_ON);
            flushLEDBuffer();
            delay(1000);
        }
    }

    // Test pattern: all on
    setAllLEDs(LED_ON);
    flushLEDBuffer();
    delay(1000);

    // Test pattern: all off
    clearAllLEDs();
    flushLEDBuffer();

    Serial.println("LED test complete");
}