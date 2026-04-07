#ifndef LED_H
#define LED_H

#include <Arduino.h>

#define LED_ON true
#define LED_OFF false

#define LED_MIN_FLUSH_MS 16

void initLEDs();
void setLED(uint8_t row, uint8_t col, bool state);
void setLEDFromInput(uint8_t expander, uint8_t pin, bool state);
void flushLEDBuffer();
void clearAllLEDs();
void setAllLEDs(bool state);
void setLEDMapping(uint8_t expander, uint8_t pin, uint8_t row, uint8_t col);
void loadDefaultMapping();
void loadCustomMapping();
void testAllLEDs();

// ========== NEW: MAPPING QUERY FUNCTIONS ==========
bool getLEDMapping(uint8_t expander, uint8_t pin, uint8_t& row, uint8_t& col);
int getLEDIndexFromRowCol(uint8_t row, uint8_t col);
int countMappedPins();
bool isInputMapped(uint8_t expander, uint8_t pin);

#endif