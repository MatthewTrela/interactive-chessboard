#include "task.h"

#include "Chess/game_manager.h"
#include "IO/io_expander.h"
#include "global.h"
#include "IO/display_manager.h"
#include "IO/encoder.h"

TaskHandle_t GameLoopTaskHandle = NULL;
TaskHandle_t UITaskHandle = NULL;
TaskHandle_t EngineTaskHandle = NULL;

void gameLoopTask(void* pvParameters) {
    uint64_t startOccupancy = readBoardBitmap();
    game.updateInitialization(startOccupancy);

    for (;;) {
        TickType_t waitTime = game.isDebouncing() ? pdMS_TO_TICKS(DEBOUNCE_MS): portMAX_DELAY;

        ulTaskNotifyTake(pdTRUE, waitTime);

        uint64_t newOccupancy = readBoardBitmap();

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
    for (uint8_t p = 0; p < 2; p++) {
        uiManager->drawMainMenu(p + 1, MenuHighlight::None);
    }

    for (;;) {
        uint32_t notificationValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));

        if (notificationValue > 0) {
            for (uint8_t p = 0; p < 2; p++) {
                EncoderData d = encoder->getData(p);
                uiManager->handleInput(p + 1, d.leftSpin, d.rightSpin, d.buttonPressed);
            }
        } 
        else {     
            if (game.getState() == SystemState::PLAYING) {
                if (game.getBoard().getSideToMove() == Chess::ChessColor::White) {
                    uiManager->updateTime(1);
                } else {
                    uiManager->updateTime(2);
                }
            }
        }
        uiManager->updateDisplays();
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
    xTaskCreatePinnedToCore(UITask, "UI", 4096, nullptr, 3, &UITaskHandle, 1);
}