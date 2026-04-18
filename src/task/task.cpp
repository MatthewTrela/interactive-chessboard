#include "task.h"
#include "global.h"
#include "IO/io_expander.h"
#include "Chess/game_manager.h"

TaskHandle_t GameLoopTaskHandle = NULL;
TaskHandle_t UITaskHandle = NULL;
TaskHandle_t EngineTaskHandle = NULL;

void gameLoopTask(void *pvParameters) {
    for(;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint64_t newOccupancy = readBoardBitmap();
        game.updateBoard(newOccupancy);
    }
}

void UITask(void *pvParameters) {
    for(;;) {
        // TODO
        // Check current game state
        // Update OLEDs based on user settings from both users
        
    }
}

void EngineTask(void *pvParameters) {
    for(;;) {
        // TODO
        // Run chess engine constantly on other core to see best moves
        
    }
}

void initTasks() {
    // TODO: Create tasks and assign priorities
    // xTaskCreatePinnedToCore(EngineTask, "Engine", 8192, nullptr, 1, &EngineTaskHandle, 0);
    xTaskCreatePinnedToCore(gameLoopTask, "GameLoop", 4096, nullptr, 5, &GameLoopTaskHandle, 1);
    
}