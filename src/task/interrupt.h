#pragma once
#include <Arduino.h>
#include <esp_attr.h>

// Initialize all physical interrupt pins
void initInterrupts();

// Handle interrupt triggered by A pin of encoder
void IRAM_ATTR encoderAInterruptHandler();

// Handle interrupt triggered by B pin of encoder
void IRAM_ATTR encoderBInterruptHandler();

// Handle interrupt triggered by button pin of encoder
void IRAM_ATTR encoderButtonInterruptHandler();

// Interrupt triggered by reed switch
void IRAM_ATTR reedSwitchInterruptHandler();