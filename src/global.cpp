#include "global.h"

#include <SPI.h>
#include <Wire.h>

// ========== OBJECT DEFINITIONS ==========
Adafruit_SSD1306* display = nullptr;
Adafruit_NeoPixel* strip = nullptr;

// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

MCP23S17* expanders[NUM_EXPANDERS] = {nullptr, nullptr, nullptr, nullptr};

// ========== VARIABLE DEFINITIONS ==========
uint16_t mcpValues[NUM_EXPANDERS] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};  // Start with all HIGH
uint16_t mcpLastValues[NUM_EXPANDERS] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
volatile bool interruptTriggered = false;

unsigned long lastInterruptTime = 0;
unsigned long lastDisplayUpdate = 0;

// ========== INITIALIZATION FUNCTION ==========
void initGlobals() {
    Wire.begin(I2C_SDA, I2C_SCL);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS_MCP);

    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    strip = new Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

    uint8_t addresses[NUM_EXPANDERS] = {MCP_ADDR_1, MCP_ADDR_2, MCP_ADDR_3, MCP_ADDR_4};
    for (int i = 0; i < NUM_EXPANDERS; i++) {
        expanders[i] = new MCP23S17(SPI_CS_MCP, addresses[i], &SPI);
    }
}