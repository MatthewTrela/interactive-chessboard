#include "task.h"

#include "Chess/game_manager.h"
#include "IO/display_manager.h"
#include "IO/encoder.h"
#include "IO/io_expander.h"
#include "IO/led.h"
#include "engine.h"
#include "esp_task_wdt.h"
#include "global.h"

TaskHandle_t GameLoopTaskHandle = NULL;
TaskHandle_t UITaskHandle = NULL;
TaskHandle_t EngineTaskHandle = NULL;

struct BestMoveHint {
    Chess::Square    fromSq;
    Chess::Square    toSq;
    Chess::ChessColor side;
    bool             valid;
};

static QueueHandle_t bestMoveQueue = NULL;

static bool    hintVisible = false;
static uint8_t hintFrom    = 0xFF;
static uint8_t hintTo      = 0xFF;

static void showHint(const BestMoveHint& hint) {
    if (!game.getSettings((hint.side == Chess::ChessColor::White) ? 0 : 1).showBestMove) {
        hintVisible = false;
        hintFrom = 0xFF;
        hintTo = 0xFF;
        return;
    }

    if (hint.valid) {
        uint8_t fromRow = hint.fromSq / 8;
        uint8_t fromCol = hint.fromSq % 8;
        uint8_t toRow   = hint.toSq   / 8;
        uint8_t toCol   = hint.toSq   % 8;

        highlightSquare(fromRow, fromCol, BEST_MOVE_FROM_COLOR);
        highlightSquare(toRow,   toCol,   BEST_MOVE_TO_COLOR);
        flushLEDBuffer(true);

        hintFrom    = hint.fromSq;
        hintTo      = hint.toSq;
        hintVisible = true;
    } else {
        hintFrom    = 0xFF;
        hintTo      = 0xFF;
        hintVisible = false;
    }
}

static void clearHint() {
    if (!hintVisible) return;
    BestMoveHint none{ Chess::SQ_NONE, Chess::SQ_NONE, Chess::ChessColor::White, false };
    showHint(none);
}

void requestEngineHint(Chess::ChessColor side) {
    if (EngineTaskHandle == NULL) {
        return;
    }

    uint32_t val = (side == Chess::ChessColor::White) ? 1u : 2u;

    xTaskNotify(EngineTaskHandle, val, eSetValueWithOverwrite);
}

// Cancel any in-progress or pending hint
void cancelEngineHint() {
    if (EngineTaskHandle == NULL) return;

    ChessEngine::abortSearch();

    if (bestMoveQueue != NULL) {
        BestMoveHint stale;
        xQueueReceive(bestMoveQueue, &stale, 0);
    }

    xTaskNotify(EngineTaskHandle, 0u, eSetValueWithOverwrite);
    hintVisible = false;
    hintFrom = 0xFF;
    hintTo = 0xFF;
}

void gameLoopTask(void* pvParameters) {
    Chess::ChessColor prevSideToMove = Chess::ChessColor::None;
    SystemState prevState = SystemState::INIT;
    MovePhase prevPhase = MovePhase::IDLE;

    for (;;) {
        SystemState state    = game.getState();
        TickType_t  waitTime;

        if (state == SystemState::GAME_OVER) {
            waitTime = pdMS_TO_TICKS(200);
        } else if (game.isDebouncing()) {
            waitTime = pdMS_TO_TICKS(DEBOUNCE_MS);
        } else {
            waitTime = 100;
        }

        ulTaskNotifyTake(pdTRUE, waitTime);

        BestMoveHint hint;
        if (bestMoveQueue != NULL && xQueueReceive(bestMoveQueue, &hint, 0) == pdTRUE) {
            Chess::ChessColor currentSide = game.getBoard().getSideToMove();
            if (hint.valid && hint.side == currentSide && game.getState() == SystemState::PLAYING && game.getMovePhase() == MovePhase::IDLE) {
                if (game.getSettings((currentSide == Chess::ChessColor::White) ? 0 : 1).showBestMove) {
                    showHint(hint);
                }
            }
        }

        uint64_t newOccupancy = readBoardBitmap();
        String reason;

        switch (state) {
            case SystemState::INIT:
                game.updateInitialization(newOccupancy);
                break;

            case SystemState::MAIN_MENU:
                break;

            case SystemState::PLAYING: {
                Chess::ChessColor currentSide = game.getBoard().getSideToMove();
                MovePhase currentPhase = game.getMovePhase();

                if (prevState != SystemState::PLAYING || currentSide != prevSideToMove) {
                    requestEngineHint(currentSide);
                    prevSideToMove = currentSide;
                }

                if (currentPhase != MovePhase::IDLE && prevPhase == MovePhase::IDLE) {
                    cancelEngineHint();
                }

                game.updateBoard(newOccupancy);
                prevPhase = currentPhase;
                break;
            }

            case SystemState::PROMOTION_SELECTION:
                if (game.getPromotionChoice() != Chess::PieceType::None) {
                    game.finalizePromotion();
                }
                break;

            case SystemState::ERROR_RECOVERY:
                game.updateBoard(newOccupancy);
                break;

            case SystemState::GAME_OVER:
                switch (game.getGameOverReason()) {
                    case GameOverReason::CHECKMATE_WHITE:
                        reason = "Checkmate by White";    
                        break;
                    case GameOverReason::CHECKMATE_BLACK:
                        reason = "Checkmate by Black";
                        break;
                    case GameOverReason::STALEMATE:
                        reason = "Stalemate";
                        break;
                    case GameOverReason::INSUFFICIENT_MATERIAL:
                        reason = "Insufficient Material"; 
                        break;
                    case GameOverReason::THREE_FOLD:
                        reason = "Three Fold Repetition"; 
                        break;
                    default:
                        reason = "None";
                }
                uiManager->notifyGameEnd(reason.c_str());
                break;

            default:
                break;
        }

        prevState = state;
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
            if (uiManager->getState(1).screen != Screen::Promotion &&
                uiManager->getState(2).screen != Screen::Promotion) {
                int player = (currentSide == Chess::ChessColor::White) ? 1 : 2;
                uiManager->startPromotion(player);
            }
        }

        if (currentState != prevState) {
            if (currentState == SystemState::PLAYING) {
                if (prevState == SystemState::ERROR_RECOVERY) {
                    if (currentSide == Chess::ChessColor::White)
                        uiManager->resumeClock(1);
                    else
                        uiManager->resumeClock(2);
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
                uiManager->updateTime(currentSide == Chess::ChessColor::White ? 1 : 2);
            }
        }

        uiManager->updateDisplays();
    }
}

static constexpr uint32_t ENGINE_SEARCH_MS = 4000;

void EngineTask(void* pvParameters) {
    esp_task_wdt_init(30, false);
    esp_task_wdt_add(NULL);

    ChessEngine::setTimeLimit(ENGINE_SEARCH_MS);

    for (;;) {
        uint32_t val = 0;
        xTaskNotifyWait(0, 0xFFFFFFFF, &val, portMAX_DELAY);
        esp_task_wdt_reset();

        if (val == 0) {
            continue;
        }

        Chess::ChessColor side = (val == 1) ? Chess::ChessColor::White : Chess::ChessColor::Black;

        Chess::Board searchBoard = game.getBoard();
        Chess::Move bestMove = ChessEngine::searchBestMove(searchBoard);
        esp_task_wdt_reset();

        uint32_t pendingVal = 0;
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &pendingVal, 0) == pdTRUE) {
            if (pendingVal == 0) {
                continue;
            } else {
                xTaskNotify(EngineTaskHandle, pendingVal, eSetValueWithOverwrite);
                continue;
            }
        }

        BestMoveHint hint;
        hint.side = side;

        if (bestMove.getFrom() == bestMove.getTo()) {
            hint.valid = false;
            hint.fromSq = Chess::SQ_NONE;
            hint.toSq = Chess::SQ_NONE;
        } else {
            hint.valid = true;
            hint.fromSq = bestMove.getFrom();
            hint.toSq = bestMove.getTo();
        }

        if (bestMoveQueue != NULL) {
            xQueueOverwrite(bestMoveQueue, &hint);
        }

        if (GameLoopTaskHandle != NULL) {
            xTaskNotifyGive(GameLoopTaskHandle);
        }
    }
}

void initTasks() {
    bestMoveQueue = xQueueCreate(1, sizeof(BestMoveHint));
    configASSERT(bestMoveQueue);

    xTaskCreatePinnedToCore(EngineTask, "Engine", 65536, nullptr, 2, &EngineTaskHandle, 0);
    xTaskCreatePinnedToCore(gameLoopTask, "GameLoop", 4096, nullptr, 5, &GameLoopTaskHandle, 1);
    xTaskCreatePinnedToCore(UITask, "UI", 4096, nullptr, 3, &UITaskHandle, 1);
}