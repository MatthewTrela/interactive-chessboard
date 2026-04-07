#include "global.h"
#include "io_expander.h"
#include "led.h"
#include "oled.h"

void setup() {
        Serial.begin(115200);
    delay(1000);

    // Initialize all global objects
    initGlobals();

    // Initialize peripherals
    initOLED();
    initLEDs();
    initExpanders();

    delay(1500);
}

void loop() {
    if (interruptTriggered) {
        // Debounce the interrupt signal
        unsigned long now = millis();
        if (now - lastInterruptTime > MCP_DEBOUNCE_DELAY_MS) {
            lastInterruptTime = now;

            // clear flag
            interruptTriggered = false;

            // find which expander triggered interrupt
            checkAllExpandersForInterrupt();
        } else {
            // false alarm
            interruptTriggered = false;
        }
    }

    // Periodically flush LED buffer to physical strip
    static unsigned long lastLEDFlush = 0;
    if (millis() - lastLEDFlush >= LED_FLUSH_INTERVAL_MS) {
        // flushLEDBuffer();
        lastLEDFlush = millis();
    }

    // Update OLED periodically
    unsigned long now = millis();
    if (now - lastDisplayUpdate >= OLED_UPDATE_INTERVAL_MS) {
        updateDisplay();
        lastDisplayUpdate = now;
    }

    delay(1);
}