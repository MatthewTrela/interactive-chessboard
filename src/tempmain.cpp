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

    // Update OLED periodically
    unsigned long now = millis();
    if (now - lastDisplayUpdate >= OLED_UPDATE_INTERVAL_MS) {
        updateDisplay();
        lastDisplayUpdate = now;
    }

    delay(1);
}