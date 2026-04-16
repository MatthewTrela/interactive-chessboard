#include "task.h"
#include "interrupt.h"
#include "global.h"

TaskHandle_t EncoderTaskHandle = NULL;
TaskHandle_t BoardSensorTaskHandle = NULL;
TaskHandle_t UITaskHandle = NULL;
TaskHandle_t EngineTaskHandle = NULL;

void EncoderTask(void *pvParameters) {
    for(;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // TODO
        // Read encoder data
        // Pass state changes to UI handler
        // Might not be nessessary
    }
}

void BoardSensorTask(void *pvParameters) {
    for(;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // read IO Expanders and find triggered reed switches
        // Show legal moves and best moves if active
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
    xTaskCreatePinnedToCore(EngineTask, "Engine", 8192, NULL, 1, &EngineTaskHandle, 0);
    
    initInterrupts();
}