#ifndef IOEXPANDER_H
#define IOEXPANDER_H

#include <MCP23S17.h>

void initExpanders(bool polling);
void checkAllExpandersForInterrupt();  // Sweep all devices
uint16_t readWithDebounce(MCP23S17* expander);
void IRAM_ATTR onExpanderInterrupt();
void poll();

#endif