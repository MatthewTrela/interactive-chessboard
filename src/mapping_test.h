// mapping_test.h - Remove processSerialCommand from here
#ifndef MAPPING_TEST_H
#define MAPPING_TEST_H

#include <Arduino.h>

#define TOTAL_INPUTS 64
#define DEBOUNCE_TIME_MS 50

void initMappingTest();
void updateMappingTest();
int countPressedInputs();

#endif