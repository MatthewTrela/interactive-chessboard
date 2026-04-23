#pragma once

#include <Arduino.h>

struct EncoderData {
    bool leftSpin;
    bool rightSpin;
    bool buttonPressed;
};

class Encoder {
public:
    Encoder();
    EncoderData getData(uint8_t player);

private:
    struct EncoderState {
        volatile int32_t delta = 0;
        volatile uint8_t prevState = 0;
        volatile bool buttonPressed = false;
        int64_t lastConsumedUs = 0;
    };

    EncoderState data[2];
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    static void IRAM_ATTR handleP1();
    static void IRAM_ATTR handleP2();
    static void IRAM_ATTR handleP1Button();
    static void IRAM_ATTR handleP2Button();

    static Encoder* instance;

    void IRAM_ATTR handle(uint8_t idx);
};