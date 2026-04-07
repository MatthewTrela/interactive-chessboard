// #include "global.h"
// #include "io_expander.h"
// #include "led.h"
// #include "mapping_test.h"
// #include "oled.h"

// void setup() {
//     Serial.begin(115200);
//     delay(5000);
//     Serial.println("Setup starting...");  // ADD THIS

//     initGlobals();

//     Serial.println("Globals initialized");  // ADD THIS

//     initOLED();

//     Serial.println("OLED initialized");  // ADD THIS

//     initLEDs();

//     Serial.println("LEDs initialized");  // ADD THIS

//     initExpanders();

//     Serial.println("Expanders initialized - Setup complete!");  // ADD THIS

//     delay(1500);
// }

// void loop() {
//     // Check for serial mode switch commands
//     if (Serial.available() > 0) {
//         char c = Serial.peek();
//         if (c == 'm' || c == 'M') {
//             Serial.read();  // Consume 'm'
//             Serial.println("\n>>> ENTERING MAPPING MODE <<<\n");
//             initMappingTest();

//             // Stay in mapping mode
//             while (true) {
//                 updateMappingTest();

//                 // Check for exit command
//                 if (Serial.available() > 0) {
//                     if (Serial.peek() == 'q') {
//                         Serial.read();
//                         break;
//                     }
//                 }
//                 delay(1);
//             }
//         }
//         // Drain any other characters
//         while (Serial.available() > 0 && Serial.peek() != 'm') {
//             Serial.read();
//         }
//     }

//     // Normal mode: handle interrupts
//     if (interruptTriggered) {
//         unsigned long now = millis();
//         if (now - lastInterruptTime > MCP_DEBOUNCE_DELAY_MS) {
//             lastInterruptTime = now;
//             interruptTriggered = false;
//             checkAllExpandersForInterrupt();
//         } else {
//             interruptTriggered = false;
//         }
//     }

//     // Flush LED buffer
//     static unsigned long lastLEDFlush = 0;
//     if (millis() - lastLEDFlush >= 16) {  // ~60fps
//         flushLEDBuffer();
//         lastLEDFlush = millis();
//     }

//     // Update OLED
//     unsigned long now = millis();
//     if (now - lastDisplayUpdate >= OLED_UPDATE_INTERVAL_MS) {
//         updateDisplay();
//         lastDisplayUpdate = now;
//     }

//     delay(1);
// }

#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("===== TEST SKETCH =====");
    Serial.println("If you see this, serial works!");
    Serial.println("========================");
}

void loop() {
    static int counter = 0;
    Serial.println("Loop running: " + String(counter++));
    delay(1000);
}