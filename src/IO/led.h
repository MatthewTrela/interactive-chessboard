#ifndef LEDS_H
#define LEDS_H

#include "global.h"
#include "io_expander.h"
#include "Chess/chess.hpp"

// LED timing constants
#define LED_FLUSH_INTERVAL_MS 30  // 33Hz refresh rate
#define LED_MIN_FLUSH_MS 10       // Throttle: max 100Hz

// LED state
#define LED_ON true
#define LED_OFF false

// Function declarations
void initLEDs();
void setLED(uint8_t row, uint8_t col, uint32_t color);
void setLEDFromInput(uint8_t expander, uint8_t pin, bool state);
void syncLEDsFromInputState();
void flushLEDBuffer();
void clearAllLEDs();
void setAllLEDs(uint32_t color);
void highlightSquare(uint8_t row, uint8_t col, uint32_t color);
void highlightLegalMoves(Chess::Square from, Chess::Board& board);

void lightAllStartingSquares();
void syncLEDsFromSensors(uint64_t sensorOccupancy);

// Mapping configuration
void setLEDMapping(uint8_t expander, uint8_t pin, uint8_t row, uint8_t col);
void loadDefaultMapping();  // Natural grid layout (exp0=rows0-1, exp1=rows2-3, etc.)
void loadCustomMapping();   // You can customize this for your orientation

// Utility functions
void testAllLEDs();                                     // Test pattern to verify all LEDs work
uint8_t getLEDIndex(uint8_t row, uint8_t col);          // Convert grid to strip index
bool getSwitchStateFromGrid(uint8_t row, uint8_t col);  // get the switch state based on row and col position

#endif