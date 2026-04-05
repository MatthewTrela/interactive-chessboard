#include "io_expander.h"

#include "global.h"

void initExpanders() {
    for (int i = 0; i < 4; i++) {
        if (!expanders[i]->begin()) {
            Serial.print("Failed to initialize Expander ");
            Serial.println(i);
            continue;
        }

        expanders[i]->enableHardwareAddress();

        expanders[i]->pinMode16(0xFFFF);
        expanders[i]->setPullup16(0xFFFF);

        expanders[i]->enableInterrupt16(0xFFFF, CHANGE);
        expanders[i]->setInterruptPolarity(2);

        // Mirror INTA and INTB (use one interrupt pin per chip)
        expanders[i]->mirrorInterrupts(true);

        // Read initial state
        mcpValues[i] = expanders[i]->read16();
        mcpLastValues[i] = mcpValues[i];

        Serial.print("Expander ");
        Serial.print(i);
        Serial.print(" (addr ");
        Serial.print(expanders[i]->getAddress());
        Serial.print(") initialized. Initial value: 0x");
        Serial.println(mcpValues[i], HEX);
    }

    // Setup shared interrupt pin
    pinMode(MCP_INT_PIN, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(MCP_INT_PIN), onExpanderInterrupt, RISING);
}

// ISR
void IRAM_ATTR onExpanderInterrupt() { interruptTriggered = true; }

// sweep function when interrupt it triggered
void checkAllExpandersForInterrupt() {
    bool anyChange = false;

    for (int i = 0; i < 4; i++) {
        uint16_t interruptFlag = expanders[i]->getInterruptFlagRegister();

        if (interruptFlag != 0) {
            uint16_t newValue = readWithDebounce(expanders[i]);

            if (newValue != mcpValues[i]) {
                // Update stored value
                mcpLastValues[i] = mcpValues[i];
                mcpValues[i] = newValue;
                anyChange = true;

                // Print which pins changed
                uint16_t changedPins = interruptFlag;
                Serial.print("Expander ");
                Serial.print(i);
                Serial.print(" changed: 0x");
                Serial.print(newValue, HEX);
                Serial.print(" (Changed pins: 0x");
                Serial.print(changedPins, HEX);
                Serial.println(")");

                // Optional: Call button/encoder handler
                // handleButtonChanges(i, changedPins, newValue);
            }
        }
    }

    if (anyChange) {
        // Force display update immediately
        lastDisplayUpdate = 0;
    }
}

uint16_t readWithDebounce(MCP23S17* expander) {
    uint16_t firstRead = expander->read16();
    delay(DEBOUNCE_DELAY_MS);
    uint16_t secondRead = expander->read16();

    if (firstRead == secondRead) {
        return firstRead;
    } else {
        // Retry if unstable
        delay(DEBOUNCE_DELAY_MS);
        return expander->read16();
    }
}