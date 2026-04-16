#include "encoder.h"
#include "global.h"
#include <FunctionalInterrupt.h>

Encoder::Encoder() {
    pinMode(ENCODER_P1_A, INPUT_PULLUP);
    pinMode(ENCODER_P1_B, INPUT_PULLUP);
    pinMode(ENCODER_P1_BUTTON, INPUT_PULLUP);

    pinMode(ENCODER_P2_A, INPUT_PULLUP);
    pinMode(ENCODER_P2_B, INPUT_PULLUP);
    pinMode(ENCODER_P2_BUTTON, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(ENCODER_P1_A), [this]() { this->p1AHandler(); }, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_P2_A), [this]() { this->p2AHandler(); }, CHANGE);
    
    attachInterrupt(digitalPinToInterrupt(ENCODER_P2_BUTTON), [this]() { this->p1ButtonHandler(); }, FALLING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_P2_BUTTON), [this]() { this->p2ButtonHandler(); }, FALLING);
}

EncoderData Encoder::getData(uint8_t player) {
    EncoderData result = {false, false, false};
    
    if (player > 1) return result; 

    portENTER_CRITICAL(&mux);
    
    result.buttonPressed = data[player].buttonPressed;
    result.rightSpin = data[player].rightSpin;
    result.leftSpin = data[player].leftSpin;
    
    data[player].buttonPressed = false;
    data[player].rightSpin = false;
    data[player].leftSpin = false;
    
    portEXIT_CRITICAL(&mux);

    return result;
}

void IRAM_ATTR Encoder::p1AHandler() {
    portENTER_CRITICAL_ISR(&mux);
    
    if (digitalRead(ENCODER_P1_A) == digitalRead(ENCODER_P1_B)) {
        data[0].rightSpin = true;
    } else {
        data[0].leftSpin = true;
    }
    
    portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR Encoder::p2AHandler() {
    portENTER_CRITICAL_ISR(&mux);
    
    if (digitalRead(ENCODER_P2_A) == digitalRead(ENCODER_P2_B)) {
        data[1].rightSpin = true;
    } else {
        data[1].leftSpin = true;
    }
    
    portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR Encoder::p1ButtonHandler() {
    portENTER_CRITICAL_ISR(&mux);
    data[0].buttonPressed = true;
    portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR Encoder::p2ButtonHandler() {
    portENTER_CRITICAL_ISR(&mux);
    data[1].buttonPressed = true;
    portEXIT_CRITICAL_ISR(&mux);
}
