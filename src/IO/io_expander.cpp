#include "io_expander.h"
#include "led.h"
#include "global.h"

bool expanderOnline[4] = {false, false, false, false};

void initExpanders(bool polling) {
    MCP23S17 bootstrap(SPI_CS_MCP, 0, &SPI);
    if (bootstrap.begin()) {
        bootstrap.enableHardwareAddress();
        delay(2);
        Serial.println("HAEN bootstrap done via addr 0");
    }

    bool anyOnline = false;

    for (int i = 0; i < 4; i++) {
        expanderOnline[i] = false;

        if (!expanders[i]->begin()) {
            Serial.printf("Failed to initialize Expander %d\n", i);
            continue;
        }

        expanders[i]->enableHardwareAddress();
        expanders[i]->pinMode16(0xFFFF);
        expanders[i]->setPullup16(0xFFFF);

        if (!polling) {
            expanders[i]->mirrorInterrupts(true);
            expanders[i]->enableInterrupt16(0xFFFF, CHANGE);
            expanders[i]->setInterruptPolarity(2);  // active-low
        }

        mcpValues[i] = expanders[i]->read16();
        mcpLastValues[i] = mcpValues[i];
        expanderOnline[i] = true;
        anyOnline = true;

        Serial.printf("Expander %d addr %d init OK, val=0x%04X\n", i, expanders[i]->getAddress(), mcpValues[i]);
    }

    if (!polling && anyOnline) {
        pinMode(MCP_INT_PIN, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(MCP_INT_PIN), onExpanderInterrupt, FALLING);
    }
}

// ISR
void IRAM_ATTR onExpanderInterrupt() { interruptTriggered = true; }

// sweep function when interrupt it triggered
void checkAllExpandersForInterrupt() {
    bool anyChange = false;

    for (int i = 0; i < 4; i++) {
        if (!expanderOnline[i]) continue;

        uint16_t intf = expanders[i]->getInterruptFlagRegister();
        if (intf == 0) continue;

        uint16_t newValue = readWithDebounce(expanders[i]);
        if (newValue != mcpValues[i]) {
            processStateChange(i, newValue);
            anyChange = true;
            Serial.printf("Expander %d changed: 0x%04X\n", i, newValue);
        }
    }

    if (anyChange) lastDisplayUpdate = 0;
}

uint16_t readWithDebounce(MCP23S17* expander) {
    uint16_t firstRead = expander->read16();
    delay(MCP_DEBOUNCE_DELAY_MS);
    uint16_t secondRead = expander->read16();

    if (firstRead == secondRead) {
        return firstRead;
    } else {
        // Retry if unstable
        delay(MCP_DEBOUNCE_DELAY_MS);
        return expander->read16();
    }
}

void processStateChange(int i, uint16_t newValue) {
    uint16_t lastVals = mcpValues[i];
    
    mcpLastValues[i] = lastVals; 
    mcpValues[i] = newValue;

    for (int j = 0; j < 16; j++) {
        uint16_t mask = 1 << j;
        
        bool isPressed = !(newValue & mask); 
        bool wasPressed = !(lastVals & mask);

        if (isPressed != wasPressed) {
            setLEDFromInput(i, j, isPressed); 
            
            if (isPressed) {
                Serial.print("ADDR: ");
                Serial.print(i);
                Serial.print(" BIT: ");
                Serial.println(j);
            }
        }
    }
}

void poll() {
    for (int i = 0; i < 4; i++) {
        if (!expanderOnline[i]) continue;
        
        uint16_t newValue = expanders[i]->read16();
        
        if (newValue != mcpValues[i]) {
            processStateChange(i, newValue);
        }
    }
}