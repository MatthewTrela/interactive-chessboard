#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <cstdint>
#include <esp_attr.h>

struct EncoderData {
    bool buttonPressed;
    bool rightSpin;
    bool leftSpin;
};

class Encoder {
public:
    Encoder();
    EncoderData getData(uint8_t player);

    void IRAM_ATTR p1AHandler();
    void IRAM_ATTR p2AHandler();
    void IRAM_ATTR p1ButtonHandler();
    void IRAM_ATTR p2ButtonHandler();

private:
    volatile EncoderData data[2] = { {false, false, false}, {false, false, false} };
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
};

#endif