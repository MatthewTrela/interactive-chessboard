#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <MCP23S17.h>
#include <SPI.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C  // Most common address - try 0x3C first
#define OLED_RESET -1

#define I2C_SDA 20
#define I2C_SCL 19

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Create expander objects directly
// Format: MCP23S17(csPin, address, &SPI)
MCP23S17 expander1(10, 0, &SPI);

uint16_t lastValues = 0;
uint16_t curVal = 0;

// for display
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 100;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("MCP23S17 + OLED Starting...");

    // I2C
    Wire.begin(I2C_SDA, I2C_SCL);

    ////////

    // Add this temporarily to setup() after Wire.begin()
    Serial.println("Scanning I2C bus...");
    int deviceCount = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.print("Found device at 0x");
            Serial.println(address, HEX);
            deviceCount++;
        }
    }
    if (deviceCount == 0) {
        Serial.println("No I2C devices found! Check wiring and pins.");
    } else {
        Serial.print("Found ");
        Serial.print(deviceCount);
        Serial.println(" I2C devices");
    }

    ////////////

    Serial.print("I2C initialized on SDA=");
    Serial.print(I2C_SDA);
    Serial.print(", SCL=");
    Serial.println(I2C_SCL);

    Serial.print("Looking for OLED at address 0x");
    Serial.print(OLED_ADDRESS, HEX);
    Serial.println("...");

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("OLED not found at 0x3C! Trying 0x3D...");

        if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
            Serial.println("OLED not found at 0x3D either!");
            Serial.println("Check your wiring and I2C pins!");
        } else {
            Serial.println("OLED initialized at 0x3D");
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
            display.println("OLED Ready!");
            display.display();
            delay(1000);
        }
    } else {
        Serial.println("OLED initialized at 0x3C");
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("OLED Ready!");
        display.display();
        delay(1000);
    }

    SPI.begin();

    // Initialize each expander
    Serial.println("Initializing Expander 1...");
    if (!expander1.begin()) {
        Serial.println("Failed to initialize Expander 1!");
    } else {
        // Set all 16 pins as inputs with pull-ups
        expander1.pinMode16(0xFFFF);    // All inputs
        expander1.setPullup16(0xFFFF);  // All pull-ups enabled
        Serial.println("Expander 1 ready");
    }
}

void drawBinaryRow(int y, const char* label, uint16_t value, bool highlight) {
    display.setCursor(0, y);
    if (highlight) {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);  // Inverted
    } else {
        display.setTextColor(SSD1306_WHITE);
    }

    display.print(label);

    // Print 16 bits with spaces every 4 bits
    for (int i = 15; i >= 0; i--) {
        if (i == 11 || i == 7 || i == 3) display.print(" ");
        display.print((value >> i) & 1);
    }
}

void loop() {
    // read all inputs
    curVal = expander1.read16();

    bool changed = (curVal != lastValues);

    if (changed) {
        Serial.print("Expander 1: ");
        // Print binary representation
        for (int i = 15; i >= 0; i--) {
            Serial.print((curVal >> i) & 1);
            if (i % 4 == 0) Serial.print(" ");
        }
        Serial.print(" (0x");
        Serial.print(curVal, HEX);
        Serial.print(") - ");

        // Count LOW pins
        int lowCount = 0;
        for (int i = 0; i < 16; i++) {
            if (!((curVal >> i) & 1)) lowCount++;
        }
        Serial.print(lowCount);
        Serial.println(" pins LOW");

        lastValues = curVal;
    }

    // Update OLED display periodically
    unsigned long now = millis();
    if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
        display.clearDisplay();

        // Title
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Input Status");
        display.drawLine(0, 8, 127, 8, SSD1306_WHITE);

        // Expander 1 - using curVal and changed
        drawBinaryRow(12, "EXP1:", curVal, changed);

        // Count LOW pins (inputs connected to GND)
        int lowCount = 0;
        for (int i = 0; i < 16; i++) {
            if (!((curVal >> i) & 1)) lowCount++;
        }

        display.setCursor(0, 30);
        display.print("LOW: ");
        display.print(lowCount);
        display.print("/16");

        // Show raw hex value
        display.setCursor(0, 45);
        display.print("HEX: 0x");
        if (curVal < 0x1000) display.print("0");  // Padding for smaller values
        if (curVal < 0x100) display.print("0");
        if (curVal < 0x10) display.print("0");
        display.print(curVal, HEX);

        // Show decimal value
        display.setCursor(0, 55);
        display.print("DEC: ");
        display.print(curVal);

        display.display();
        lastDisplayUpdate = now;
    }

    delay(10);  // Small delay to prevent overwhelming
}