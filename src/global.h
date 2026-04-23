#ifndef GLOBAL_H
#define GLOBAL_H

#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <MCP23S17.h>

class DisplayManager;
class Encoder;

// I2C Pins
#define P1_SDA 4
#define P1_SCL 5
#define P2_SDA 1
#define P2_SCL 2

// SPI Pins
// #define SPI_MOSI 13
// #define SPI_MISO 11
// #define SPI_SCK 12
// #define SPI_CS_MCP 10  // Shared CS for all MCP23S17
// #define MCP_INT_PIN 14
#define SPI_MOSI 36
#define SPI_MISO 37
#define SPI_SCK 35
#define SPI_CS_MCP 47  // Shared CS for all MCP23S17
#define MCP_INT_PIN 38

// Encoder pins
#define ENCODER_P1_A 6
#define ENCODER_P1_B 7
#define ENCODER_P1_BUTTON 15
#define ENCODER_P2_A 40
#define ENCODER_P2_B 41
#define ENCODER_P2_BUTTON 42

// MCP23S17
#define NUM_EXPANDERS 4

#define MCP_ADDR_1 0  // 000
#define MCP_ADDR_2 1  // 001
#define MCP_ADDR_3 2  // 010
#define MCP_ADDR_4 3  // 011

#define MCP_DEBOUNCE_DELAY_MS 15
#define DEBOUNCE_MS 50

// OLED
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_TITLE_Y 0
#define OLED_LINE_Y 8
#define OLED_EXPANDER_ROW_HEIGHT 12
#define OLED_FIRST_EXPANDER_Y 12
#define OLED_STATUS_Y 58

#define OLED_UPDATE_INTERVAL_MS 100

// LED Strip
#define LED_PIN 48
#define NUM_LEDS 64
#define LED_BRIGHTNESS 255
#define LEGAL_MOVE_COLOR 0x0000FF     // blue
#define LEGAL_CAPTURE_COLOR 0xFF6600  // orange
#define LIFTED_PIECE_COLOR 0xFFFFFF   // white
#define CASTLING_COLOR 0xBF00FF       // PURPLE
#define WHITE_PIECES 0xFFFFFF         // white
#define BLACK_PIECES 0xFF0000         // red

// ========== GLOBAL OBJECTS (Declared as extern) ==========
extern DisplayManager* uiManager;
extern Encoder* encoder;
extern Adafruit_NeoPixel* strip;
extern MCP23S17* expanders[4];

// expander state
typedef union {
    uint16_t expander[4];
    uint64_t raw;
} ExpanderState;

// ========== GLOBAL VARIABLES ==========
extern ExpanderState expanderState;
extern ExpanderState expanderLastState;
extern volatile bool interruptTriggered;

extern unsigned long lastInterruptTime;
// extern unsigned long lastDisplayUpdate;

// ========== FUNCTION DECLARATIONS ==========
void initGlobals();  // Call this in setup() to initialize all objects

#endif