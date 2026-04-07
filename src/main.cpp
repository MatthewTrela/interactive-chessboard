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
    initExpanders(false);

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
        flushLEDBuffer();
        lastLEDFlush = millis();
    }

    // Update OLED periodically
    unsigned long now = millis();
    if (now - lastDisplayUpdate >= OLED_UPDATE_INTERVAL_MS) {
        poll();
        updateDisplay();
        lastDisplayUpdate = now;
    }

    delay(1);
}

// #include <Arduino.h>
// #include <SPI.h>

// static const int PIN_MOSI = 11;
// static const int PIN_MISO = 13;
// static const int PIN_SCK = 12;
// static const int PIN_CS = 10;

// SPIClass spi(FSPI);

// uint8_t opcodeWrite(uint8_t addr) { return 0x40 | ((addr & 0x07) << 1) | 0; }
// uint8_t opcodeRead(uint8_t addr) { return 0x40 | ((addr & 0x07) << 1) | 1; }

// // BANK=0 register map
// static const uint8_t REG_IODIRA = 0x00;
// static const uint8_t REG_IODIRB = 0x01;
// static const uint8_t REG_IOCON = 0x0A;  // also 0x0B
// static const uint8_t REG_GPPUA = 0x0C;
// static const uint8_t REG_GPPUB = 0x0D;
// static const uint8_t REG_GPIOA = 0x12;
// static const uint8_t REG_GPIOB = 0x13;

// const uint8_t ADDRS[] = {0, 1, 2, 3};
// uint16_t lastState[4] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

// uint8_t mcpReadRegAt(uint8_t addr, uint8_t reg) {
//     digitalWrite(PIN_CS, LOW);
//     spi.transfer(opcodeRead(addr));
//     spi.transfer(reg);
//     uint8_t v = spi.transfer(0x00);
//     digitalWrite(PIN_CS, HIGH);
//     return v;
// }

// void mcpWriteRegAt(uint8_t addr, uint8_t reg, uint8_t val) {
//     digitalWrite(PIN_CS, LOW);
//     spi.transfer(opcodeWrite(addr));
//     spi.transfer(reg);
//     spi.transfer(val);
//     digitalWrite(PIN_CS, HIGH);
// }

// void setup() {
//     Serial.begin(115200);
//     delay(300);
//     Serial.println("\n=== MCP23S17 4-EXPANDER SHARED-CS TEST ===");

//     pinMode(PIN_CS, OUTPUT);
//     digitalWrite(PIN_CS, HIGH);

//     spi.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
//     spi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

//     // 1) Bootstrap HAEN once (all chips see this when HAEN=0)
//     mcpWriteRegAt(0, REG_IOCON, 0x08);
//     mcpWriteRegAt(0, 0x0B, 0x08);
//     delay(2);

//     // 2) Init each addressed chip
//     for (int i = 0; i < 4; i++) {
//         uint8_t a = ADDRS[i];

//         mcpWriteRegAt(a, REG_IODIRA, 0xFF);  // inputs
//         mcpWriteRegAt(a, REG_IODIRB, 0xFF);
//         mcpWriteRegAt(a, REG_GPPUA, 0xFF);  // pullups on
//         mcpWriteRegAt(a, REG_GPPUB, 0xFF);

//         uint8_t iocon = mcpReadRegAt(a, REG_IOCON);
//         uint8_t dira = mcpReadRegAt(a, REG_IODIRA);
//         uint8_t dirb = mcpReadRegAt(a, REG_IODIRB);

//         Serial.printf("ADDR %u: IOCON=0x%02X IODIRA=0x%02X IODIRB=0x%02X\n", a, iocon, dira, dirb);
//     }

//     Serial.println("Short any GPA/GPB pin to GND; matching bit should go 0.");
// }

// void loop() {
//     for (int i = 0; i < 4; i++) {
//         uint8_t a = ADDRS[i];
//         uint8_t ga = mcpReadRegAt(a, REG_GPIOA);
//         uint8_t gb = mcpReadRegAt(a, REG_GPIOB);
//         uint16_t state = ((uint16_t)gb << 8) | ga;

//         if (state != lastState[i]) {
//             Serial.printf("ADDR %u  GPIOA=0x%02X GPIOB=0x%02X\n", a, ga, gb);
//             lastState[i] = state;
//         }
//     }
//     delay(50);
// }