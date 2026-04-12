#include "oled.h"

#include <Adafruit_GFX.h>

#include "global.h"
#include "led.h"  // for getswitchstate method

// Track previous values to detect changes
static uint16_t lastDisplayedValues[NUM_EXPANDERS] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
static bool firstRun = true;

void initOLED() {
    Serial.println("Initializing OLED...");

    if (!display->begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("OLED allocation failed!");
        return;
    }

    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->println("OLED Ready!");
    display->display();

    Serial.println("OLED initialized successfully");
    delay(1000);
}

void drawBinaryRow(int y, int expanderNum, uint16_t value, bool highlight) {
    // Set text color (inverted if highlighted)
    if (highlight) {
        display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        display->setTextColor(SSD1306_WHITE);
    }

    // Print expander label
    display->setCursor(0, y);
    display->print("E");
    display->print(expanderNum);
    display->print(":");

    // Print 16 bits with spaces every 4 bits
    for (int i = 15; i >= 0; i--) {
        if (i == 11 || i == 7 || i == 3) {
            display->print(" ");
        }
        display->print((value >> i) & 1);
    }
}

void displayDebugMessage(const char* line1, const char* line2) {
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->println(line1);

    if (line2 != nullptr) {
        display->setCursor(0, 10);
        display->println(line2);
    }

    display->display();
}

// void updateDisplay() {
//     if (display == nullptr) return;

//     bool anyChanged = false;

//     // Check if any expander value changed since last display update
//     for (int i = 0; i < NUM_EXPANDERS; i++) {
//         if (mcpValues[i] != lastDisplayedValues[i]) {
//             anyChanged = true;
//             lastDisplayedValues[i] = mcpValues[i];
//         }
//     }

//     // Only redraw if changes occurred or first run
//     if (!anyChanged && !firstRun) {
//         return;
//     }
//     firstRun = false;

//     display->clearDisplay();

//     // ===== TITLE SECTION =====
//     display->setTextSize(1);
//     display->setTextColor(SSD1306_WHITE);
//     display->setCursor(0, OLED_TITLE_Y);
//     display->println("I/O Expander Status");
//     display->drawLine(0, OLED_LINE_Y, SCREEN_WIDTH - 1, OLED_LINE_Y, SSD1306_WHITE);

//     // ===== EXPANDER ROWS =====
//     for (int i = 0; i < NUM_EXPANDERS; i++) {
//         int y = OLED_FIRST_EXPANDER_Y + (i * OLED_EXPANDER_ROW_HEIGHT);
//         bool highlight = (mcpValues[i] != mcpLastValues[i]);
//         drawBinaryRow(y, i, mcpValues[i], highlight);
//     }

//     // ===== STATUS LINE =====
//     display->setTextColor(SSD1306_WHITE);
//     display->setCursor(0, OLED_STATUS_Y);

//     // Count total LOW pins across all expanders
//     int totalLowPins = 0;
//     for (int i = 0; i < NUM_EXPANDERS; i++) {
//         for (int pin = 0; pin < 16; pin++) {
//             if (!((mcpValues[i] >> pin) & 1)) {
//                 totalLowPins++;
//             }
//         }
//     }

//     display->print("LOW:");
//     display->print(totalLowPins);
//     display->print("/");
//     display->print(NUM_EXPANDERS * 16);

//     // Show interrupt status
//     display->setCursor(80, OLED_STATUS_Y);
//     display->print("INT:");
//     display->print(interruptTriggered ? "Pend" : "OK");

//     display->display();
// }

void updateDisplay() {
    display->clearDisplay();

    // Draw title
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);

    // Draw 8x8 grid
    // 128 wide, leave margins → 14px per square + 1px gap = 15px
    const uint8_t CELL = 7;
    const uint8_t GAP = 1;
    const uint8_t CELL_WITH_GAP = CELL + GAP;
    const uint8_t GRID_START_X = 4;
    const uint8_t GRID_START_Y = 0;

    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 8; col++) {
            bool pressed = getSwitchStateFromGrid(row, col);

            uint8_t x = GRID_START_X + col * CELL_WITH_GAP;
            uint8_t y = GRID_START_Y + (7 - row) * CELL_WITH_GAP;  // flip Y so row 0 = top

            if (pressed) {
                display->fillRect(x, y, CELL, CELL, SSD1306_WHITE);
            } else {
                display->drawRect(x, y, CELL, CELL, SSD1306_WHITE);
            }
        }
    }

    display->display();
}

// Compact view showing only LOW counts per expander
void updateDisplayCompact() {
    if (display == nullptr) return;

    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);

    // Title
    display->setCursor(0, OLED_TITLE_Y);
    display->println("Expander Status");
    display->drawLine(0, OLED_LINE_Y, 127, OLED_LINE_Y, SSD1306_WHITE);

    // Show LOW pin counts for each expander
    for (int i = 0; i < NUM_EXPANDERS; i++) {
        int y = 12 + (i * 12);
        int lowCount = 0;

        for (int pin = 0; pin < 16; pin++) {
            if (!((mcpValues[i] >> pin) & 1)) lowCount++;
        }

        display->setCursor(0, y);
        display->print("E");
        display->print(i);
        display->print(": ");

        // Progress bar (max 16 low pins)
        int barWidth = map(lowCount, 0, 16, 0, 80);
        display->fillRect(25, y - 2, barWidth, 6, SSD1306_WHITE);

        display->setCursor(110, y);
        display->print(lowCount);
    }

    // Bottom status
    display->setCursor(0, OLED_STATUS_Y);
    display->print("INT:");
    display->print(interruptTriggered ? "1" : "0");

    display->display();
}
