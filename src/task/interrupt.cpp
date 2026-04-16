#include "interrupt.h"
#include "task.h" 
#include "global.h"

void initInterrupts() {
    // TODO: Attatch all interrupt pins to correct interrupt handler

}

void IRAM_ATTR encoderAInterruptHandler() {
    // TODO: Determine direction turned and send appropriate turn info to UI handler

}

void IRAM_ATTR encoderBInterruptHandler() {
    // TODO: Determine direction turned and send appropriate turn info to UI handler

}

void IRAM_ATTR encoderButtonInterruptHandler() {
    // TODO: Tell UI  button was pressed

}

void IRAM_ATTR reedSwitchInterruptHandler() {
    // TODO: Figure out which reed switch was triggered and tell IO expander class

}