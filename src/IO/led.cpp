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

const uint32_t base_color = 0x00FF00;

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

    return !(expanderState.expander[expander - 1] & (1 << idx));
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
        uint16_t value = expanderState.expander[i];

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
void flushLEDBuffer(bool force) {
    unsigned long now = millis();

    // throttle update
    if (!force && now - lastFlushTime < LED_MIN_FLUSH_MS) {
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
void highlightSquare(uint8_t row, uint8_t col, uint32_t color) {
    if (row < 0 || col < 0 || row >= 8 || col >= 8) return;
    ledBuffer[row][col] = color;
    ledDirty[row][col] = true;
}

void highlightLegalMoves(Chess::Square from, Chess::Board& board) {
    clearAllLEDs();

    // highlight lifted piece
    uint8_t fromRow = from / 8;
    uint8_t fromCol = from % 8;
    highlightSquare(fromRow, fromCol, LIFTED_PIECE_COLOR);

    Chess::MoveList moves;
    board.generateLegalMoves(moves);

    for (size_t i = 0; i < moves.size(); i++) {
        if (moves[i].getFrom() != from) continue;

        Chess::Square to = moves[i].getTo();
        uint8_t row = to / 8;
        uint8_t col = to % 8;

        uint32_t color;
        if (moves[i].getFlags() == 2 || moves[i].getFlags() == 3) {
            color = CASTLING_COLOR;
        } else if (moves[i].isCapture()) {
            color = LEGAL_CAPTURE_COLOR;
        } else {
            color = LEGAL_MOVE_COLOR;
        }
        highlightSquare(row, col, color);
    }

    flushLEDBuffer();
}

void lightAllStartingSquares(uint64_t sensorOccupancy) {
    delay(10);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            uint8_t sq = row * 8 + col;
            bool isStartingSquare = (row <= 1 || row >= 6);
            bool hasPiece = (sensorOccupancy >> sq) & 1ULL;

            // Only light if it's a starting square AND piece is missing
            if (isStartingSquare && !hasPiece) {
                setLED(row, col, (row <= 1) ? WHITE_PIECES : BLACK_PIECES);
            } else {
                setLED(row, col, 0x000000);
            }
        }
    }
    flushLEDBuffer();
}

void clearAllLEDs() {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            ledBuffer[row][col] = 0x000000;
            ledDirty[row][col] = true;
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