#include "task.h"

#include "Chess/game_manager.h"
#include "IO/io_expander.h"
#include "global.h"

TaskHandle_t GameLoopTaskHandle = NULL;
TaskHandle_t UITaskHandle = NULL;
TaskHandle_t EngineTaskHandle = NULL;

void gameLoopTask(void* pvParameters) {
    for (;;) {
        poll();
        // ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint64_t newOccupancy = readBoardBitmap();
        // uiManager->drawGrid(1);
        // uiManager->drawGrid(2);
        game.updateBoard(newOccupancy);
        vTaskDelay(pdMS_TO_TICKS(10));
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