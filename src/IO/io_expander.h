#ifndef IOEXPANDER_H
#define IOEXPANDER_H

#include <MCP23S17.h>
#include <stdbool.h>
#include <stdint.h>

// mapping table
struct InputRef {
    uint8_t exp1;  // 1-4
    uint8_t pin1;  // 1-16
};

extern const InputRef switchGrid[8][8];

static const uint8_t swToIdx[17] = {
    0xFF,
    13,  // SW1  = GPA5
    12,  // SW2  = GPA4
    11,  // SW3  = GPA3
    10,  // SW4  = GPA2
    15,  // SW5  = GPA7
    14,  // SW6  = GPA6
    8,   // SW7  = GPA0
    9,   // SW8  = GPA1
    0,   // SW9  = GPB0
    1,   // SW10 = GPB1
    5,   // SW11 = GPB5
    7,   // SW12 = GPB7
    2,   // SW13 = GPB2
    3,   // SW14 = GPB3
    4,   // SW15 = GPB4
    6    // SW16 = GPB6
};

void initExpanders(bool polling);
uint64_t checkAllExpandersForInterrupt();  // Sweep all devices
uint16_t readWithDebounce(MCP23S17* expander);
void IRAM_ATTR onExpanderInterrupt();
void processStateChange(int i, uint16_t newValue);
void poll();

// bitmap funcs
uint64_t readBoardBitmap();
bool getSquareOccupied(uint8_t row, uint8_t col);
void printBoardState();

#endif