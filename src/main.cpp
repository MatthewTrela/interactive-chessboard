#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <MCP23S17.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C  // Most common address - try 0x3C first
#define OLED_RESET -1

#define I2C_SDA 20
#define I2C_SCL 19

#define MCP_INT_PIN 1

#define LED_PIN 5
#define NUM_LEDS 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Create expander objects directly
// Format: MCP23S17(csPin, address, &SPI)
MCP23S17 expander1(10, 0, &SPI);

uint16_t lastValues = 0;
uint16_t curVal = 0;
uint16_t debouncedValues = 0;

// For interrupt handling
volatile bool interruptTriggered = false;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// for debouncing
unsigned long lastInterruptTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 15;

// for display
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 100;

void IRAM_ATTR onExpanderInterrupt() {
    portENTER_CRITICAL_ISR(&mux);
    interruptTriggered = true;
    portEXIT_CRITICAL_ISR(&mux);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("MCP23S17 + OLED Starting...");

    // I2C
    Wire.begin(I2C_SDA, I2C_SCL);

    Serial.print("I2C initialized on SDA=");
    Serial.print(I2C_SDA);
    Serial.print(", SCL=");
    Serial.println(I2C_SCL);

    Serial.print("Looking for OLED at address 0x");
    Serial.print(OLED_ADDRESS, HEX);
    Serial.println("...");

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("OLED not found at 0x3C!");
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

        expander1.enableInterrupt16(0xFFFF, CHANGE);  // enable interrupts for all pins

        expander1.setInterruptPolarity(1);  // Active HIGH

        // initial state
        debouncedValues = expander1.read16();
        lastValues = debouncedValues;
        curVal = debouncedValues;

        // interrupt pin
        pinMode(MCP_INT_PIN, INPUT_PULLDOWN);
        expander1.mirrorInterrupts(true);

        attachInterrupt(digitalPinToInterrupt(MCP_INT_PIN), onExpanderInterrupt, RISING);

        Serial.println("Expander 1 ready");
    }

    strip.begin();
    strip.setBrightness(50);
    strip.fill(strip.Color(255, 255, 255)); // white
    strip.show();
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

uint16_t readWithDebounce() {
    uint16_t firstRead = expander1.read16();
    delay(DEBOUNCE_DELAY_MS);
    uint16_t secondRead = expander1.read16();

    if (firstRead == secondRead) {
        return firstRead;
    } else {
        // retry if unstable
        delay(DEBOUNCE_DELAY_MS);
        return expander1.read16();
    }
}

void loop() {
    if (interruptTriggered) {
        portENTER_CRITICAL(&mux);
        interruptTriggered = false;
        portEXIT_CRITICAL(&mux);

        unsigned long curTime = millis();

        // debounce interrupt
        if (curTime - lastInterruptTime > DEBOUNCE_DELAY_MS) {
            lastInterruptTime = curTime;

            // read what pins caused interrupt, clear flags
            uint16_t interruptFlag = expander1.getInterruptFlagRegister();
            uint16_t interruptCapture = expander1.getInterruptCaptureRegister();

            uint16_t newValues = readWithDebounce();

            // Update if changed and stable
            if (newValues != debouncedValues) {
                debouncedValues = newValues;
                curVal = debouncedValues;  // Update display value

                // Print to serial
                Serial.print("Interrupt! Changed pins: 0x");
                Serial.print(interruptFlag, HEX);
                Serial.print(" New value: ");
                for (int i = 15; i >= 0; i--) {
                    Serial.print((curVal >> i) & 1);
                    if (i % 4 == 0) Serial.print(" ");
                }
            }
        }
    }

    // update OLED periodically
    unsigned long curTime = millis();
    if (curTime - lastDisplayUpdate >= DISPLAY_INTERVAL) {
        bool changed = (curVal != lastValues);

        display.clearDisplay();

        // Title
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Input Status");
        display.drawLine(0, 8, 127, 8, SSD1306_WHITE);

        // Expander 1
        drawBinaryRow(12, "", curVal, changed);

        // Count LOW pins
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
        if (curVal < 0x1000) display.print("0");
        if (curVal < 0x100) display.print("0");
        if (curVal < 0x10) display.print("0");
        display.print(curVal, HEX);

        display.display();
        lastDisplayUpdate = curTime;
        lastValues = curVal;  // update last values
    }

    delay(1);
}