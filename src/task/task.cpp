#include "task.h"

#include "Chess/game_manager.h"
#include "IO/io_expander.h"
#include "global.h"
#include "IO/display_manager.h"

TaskHandle_t GameLoopTaskHandle = NULL;
TaskHandle_t UITaskHandle = NULL;
TaskHandle_t EngineTaskHandle = NULL;

void gameLoopTask(void* pvParameters) {
    for (;;) {
        TickType_t waitTime = game.isDebouncing()
            ? pdMS_TO_TICKS(DEBOUNCE_MS)
            : portMAX_DELAY;

        ulTaskNotifyTake(pdTRUE, waitTime);

        uint64_t newOccupancy = readBoardBitmap();
        uiManager->drawGrid(1, newOccupancy);
        uiManager->drawGrid(2, newOccupancy);

        switch (game.getState()) {
            case SystemState::INIT:
                game.updateInitialization(newOccupancy);
                break;
            case SystemState::PLAYING:
                Serial.println("We in playing");
                game.updateBoard(newOccupancy);
                break;
            case SystemState::ERROR_RECOVERY:
                // TODO: show error on OLED
                Serial.println("We in error");
                break;
            case SystemState::GAME_OVER:
                // TODO: show game over
                Serial.println("We in game over");
                break;
            default:
                break;
        }
    }
}

void UITask(void* pvParameters) {
    for (;;) {
        // TODO
        // Check current game state
        // Update OLEDs based on user settings from both users
    }
}

void EngineTask(void* pvParameters) {
    for (;;) {
        // TODO
        // Run chess engine constantly on other core to see best moves
    }
}

void initTasks() {
    // TODO: Create tasks and assign priorities
    // xTaskCreatePinnedToCore(EngineTask, "Engine", 8192, nullptr, 1, &EngineTaskHandle, 0);
    xTaskCreatePinnedToCore(gameLoopTask, "GameLoop", 4096, nullptr, 5, &GameLoopTaskHandle, 1);
}