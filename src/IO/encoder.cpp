#include "encoder.h"
#include "global.h"
#include <FunctionalInterrupt.h>
#include "task/task.h"

Encoder* Encoder::instance = nullptr;

static inline void notifyUITask() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (UITaskHandle) {
        vTaskNotifyGiveFromISR(UITaskHandle, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static const int8_t transitionTable[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

Encoder::Encoder() {
    instance = this;

    pinMode(ENCODER_P1_A, INPUT_PULLUP);
    pinMode(ENCODER_P1_B, INPUT_PULLUP);
    pinMode(ENCODER_P1_BUTTON, INPUT_PULLUP);

    pinMode(ENCODER_P2_A, INPUT_PULLUP);
    pinMode(ENCODER_P2_B, INPUT_PULLUP);
    pinMode(ENCODER_P2_BUTTON, INPUT_PULLUP);

    data[0].prevState = (digitalRead(ENCODER_P1_A) << 1) | digitalRead(ENCODER_P1_B);

    data[1].prevState = (digitalRead(ENCODER_P2_A) << 1) |digitalRead(ENCODER_P2_B);

    attachInterrupt(digitalPinToInterrupt(ENCODER_P1_A), handleP1, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_P1_B), handleP1, CHANGE);

    attachInterrupt(digitalPinToInterrupt(ENCODER_P2_A), handleP2, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_P2_B), handleP2, CHANGE);

    attachInterrupt(digitalPinToInterrupt(ENCODER_P1_BUTTON), handleP1Button, FALLING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_P2_BUTTON), handleP2Button, FALLING);
}

void IRAM_ATTR Encoder::handle(uint8_t idx) {
    uint8_t a, b;

    if (idx == 0) {
        a = digitalRead(ENCODER_P1_A);
        b = digitalRead(ENCODER_P1_B);
    } else {
        a = digitalRead(ENCODER_P2_A);
        b = digitalRead(ENCODER_P2_B);
    }

    uint8_t curr = (a << 1) | b;

    portENTER_CRITICAL_ISR(&mux);

    uint8_t prev = data[idx].prevState;
    uint8_t transition = (prev << 2) | curr;

    int8_t movement = transitionTable[transition];

    data[idx].steps += movement;
    data[idx].prevState = curr;

    portEXIT_CRITICAL_ISR(&mux);

    if (movement != 0) {
        notifyUITask();
    }
}

// Static ISR wrappers
void IRAM_ATTR Encoder::handleP1() { instance->handle(0); }
void IRAM_ATTR Encoder::handleP2() { instance->handle(1); }

void IRAM_ATTR Encoder::handleP1Button() {
    if (digitalRead(ENCODER_P1_BUTTON) != LOW) return;

    int64_t now = millis();
    bool registered = false;
    
    portENTER_CRITICAL_ISR(&instance->mux);
    if ((now - instance->data[0].lastButtonMs) >= BUTTON_DEBOUNCE_MS) {
        instance->data[0].buttonPressed = true;
        instance->data[0].lastButtonMs  = now;
        registered = true;
    }
    portEXIT_CRITICAL_ISR(&instance->mux);

    if (registered) {
        Serial.println("Button pressed");
        notifyUITask();
    }
}

void IRAM_ATTR Encoder::handleP2Button() {
    if (digitalRead(ENCODER_P2_BUTTON) != LOW) return;

    int64_t now = millis();
    portENTER_CRITICAL_ISR(&instance->mux);
    if ((now - instance->data[1].lastButtonMs) >= BUTTON_DEBOUNCE_MS) {
        instance->data[1].buttonPressed = true;
        instance->data[1].lastButtonMs  = now;
    }
    portEXIT_CRITICAL_ISR(&instance->mux);
    if (instance->data[1].buttonPressed) {
        notifyUITask();
    }
    
}

EncoderData Encoder::getData(uint8_t player) {
    EncoderData result = {false, false, false};
    if (player > 1) return result;

    portENTER_CRITICAL(&mux);

    int32_t s   = data[player].steps;
    bool    btn = data[player].buttonPressed;

    if (s >= 4) {
        data[player].steps -= 4;
        result.rightSpin = true;
    } else if (s <= -4) {
        data[player].steps += 4;
        result.leftSpin = true;
    }

    if (btn) data[player].buttonPressed = false;

    portEXIT_CRITICAL(&mux);

    result.buttonPressed = btn;
    return result;
}