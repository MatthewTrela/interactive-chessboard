#include "global.h"
#include "IO/io_expander.h"
#include "IO/led.h"
#include "IO/oled.h"
#include "Chess/chess.hpp"
#include "task/task.h"
#include "Chess/game_manager.h"
#include "IO/display_manager.h"

using namespace Chess;

Board board;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initialize all global objects
    initGlobals();
    game.init();

    if (!uiManager->begin()) {
        Serial.println("OLEDs failed to initialize!");
    }

    // Initialize peripherals
    initLEDs();
    initExpanders(false);

    // set initial led state
    syncLEDsFromInputState();

    //Setup tasks
    initTasks();
}

void loop() {

    vTaskDelete(NULL);
}