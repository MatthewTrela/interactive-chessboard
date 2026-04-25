#include "task.h"

#include "Chess/game_manager.h"
#include "IO/display_manager.h"
#include "IO/encoder.h"
#include "IO/io_expander.h"
#include "global.h"

TaskHandle_t GameLoopTaskHandle = NULL;
TaskHandle_t UITaskHandle = NULL;
TaskHandle_t EngineTaskHandle = NULL;

void gameLoopTask(void* pvParameters) {
    for (;;) {
        SystemState state = game.getState();
        TickType_t waitTime;

        if (state == SystemState::GAME_OVER) {
            waitTime = pdMS_TO_TICKS(200);
        } else if (game.isDebouncing()) {
            waitTime = pdMS_TO_TICKS(DEBOUNCE_MS);
        } else {
            waitTime = portMAX_DELAY;
        }

        ulTaskNotifyTake(pdTRUE, waitTime);

        uint64_t newOccupancy = readBoardBitmap();

        switch (game.getState()) {
            case SystemState::INIT:
                game.updateInitialization(newOccupancy);
                break;
            case SystemState::MAIN_MENU:

                break;
            case SystemState::PLAYING:
                game.updateBoard(newOccupancy);
                break;
            case SystemState::PROMOTION_SELECTION:
                // The UI task will set promotionChoice and notify us.
                if (game.getPromotionChoice() != Chess::PieceType::None) {
                    game.finalizePromotion();
                }
                break;
            case SystemState::ERROR_RECOVERY:
                // TODO: show error on OLED
                game.updateBoard(newOccupancy);
                break;
            case SystemState::GAME_OVER:
                // TODO: show game over
                uiManager->notifyGameEnd("reason");
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

    SystemState prevState = game.getState();
    Chess::ChessColor prevSide = Chess::ChessColor::White;
    bool blackClockStarted = false;

    for (;;) {
        uint32_t notificationValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));

        SystemState currentState = game.getState();
        Chess::ChessColor currentSide = game.getBoard().getSideToMove();

        if (currentState == SystemState::PROMOTION_SELECTION) {
            // Set up the promotion screen once (if not already showing)
            if (uiManager->getState(1).screen != Screen::Promotion &&
                uiManager->getState(2).screen != Screen::Promotion) {
                int player = (currentSide == Chess::ChessColor::White) ? 1 : 2;
                uiManager->startPromotion(player);
            }
        }

        if (currentState != prevState) {
            if (currentState == SystemState::PLAYING) {
                if (prevState == SystemState::ERROR_RECOVERY) {
                    if (currentSide == Chess::ChessColor::White) {
                        uiManager->resumeClock(1);
                    } else {
                        uiManager->resumeClock(2);
                    }
                } else {
                    blackClockStarted = false;
                    prevSide = Chess::ChessColor::White;
                    uiManager->startClock(1);
                }
            } else if (currentState == SystemState::ERROR_RECOVERY) {
                uiManager->pauseClock(1);
                uiManager->pauseClock(2);
            }
            prevState = currentState;
        }

        if (currentState == SystemState::PLAYING && currentSide != prevSide) {
            if (currentSide == Chess::ChessColor::Black) {
                uiManager->pauseClock(1);
                if (!blackClockStarted) {
                    uiManager->startClock(2);
                    blackClockStarted = true;
                } else {
                    uiManager->resumeClock(2);
                }
            } else {
                uiManager->pauseClock(2);
                uiManager->resumeClock(1);
            }
        }
        prevSide = currentSide;

        if (notificationValue > 0) {
            for (uint8_t p = 0; p < 2; p++) {
                EncoderData d = encoder->getData(p);
                uiManager->handleInput(p + 1, d.leftSpin, d.rightSpin, d.buttonPressed);
                if (uiManager->isPromotionComplete()) {
                    game.setPromotionChoice(uiManager->getPromotionResult());
                    uiManager->clearPromotionState();
                    xTaskNotifyGive(GameLoopTaskHandle);
                }
            }
        } else {
            if (currentState == SystemState::PLAYING) {
                if (currentSide == Chess::ChessColor::White) {
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