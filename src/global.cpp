#include "global.h"

#include <SPI.h>
#include <Wire.h>

#include "IO/display_manager.h"
#include "IO/encoder.h"

Adafruit_NeoPixel* strip = nullptr;
DisplayManager* uiManager = nullptr;
Encoder* encoder = nullptr;
MCP23S17* expanders[NUM_EXPANDERS] = {nullptr, nullptr, nullptr, nullptr};

ExpanderState expanderState = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
ExpanderState expanderLastState = {0, 0, 0, 0};

volatile bool interruptTriggered = false;
unsigned long lastInterruptTime = 0;
// unsigned long lastDisplayUpdate = 0;

// ========== INITIALIZATION FUNCTION ==========
void initGlobals() {
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS_MCP);

    strip = new Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
    uiManager = new DisplayManager();
    encoder = new Encoder();

    uint8_t addresses[NUM_EXPANDERS] = {MCP_ADDR_1, MCP_ADDR_2, MCP_ADDR_3, MCP_ADDR_4};
    for (int i = 0; i < NUM_EXPANDERS; i++) {
        expanders[i] = new MCP23S17(SPI_CS_MCP, addresses[i], &SPI);
    }
}