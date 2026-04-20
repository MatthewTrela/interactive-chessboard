#include "Chess/chess.hpp"
#include "Chess/game_manager.h"
#include "IO/display_manager.h"
#include "IO/io_expander.h"
#include "IO/led.h"
#include "IO/oled.h"
#include "global.h"
#include "task/task.h"

using namespace Chess;

Board board;

void setup() {
    Serial.begin(115200);

    while (!Serial);
    delay(3000);

    Serial.println("Hello from ESP32-S3!");
    // Initialize all global objects
    initGlobals();
    game.init();

    if (!uiManager->begin()) {
        Serial.println("OLEDs failed to initialize!");
    }

    // Initialize peripherals
    initLEDs();
    initExpanders(true);

    // Draw on OLEDs
    uiManager->drawGrid(1);
    uiManager->drawGrid(2);

    // set initial led state
    syncLEDsFromInputState();

    // Setup tasks
    initTasks();
}

void loop() { vTaskDelete(NULL); }