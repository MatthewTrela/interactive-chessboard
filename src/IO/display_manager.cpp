#include "display_manager.h"

#include "Chess/game_manager.h"
#include "IO/io_expander.h"
#include "IO/led.h"

static const char* pieceTypeName(Chess::PieceType pt) {
    switch (pt) {
        case Chess::PieceType::Pawn:
            return "Pawn";
        case Chess::PieceType::Knight:
            return "Knight";
        case Chess::PieceType::Bishop:
            return "Bishop";
        case Chess::PieceType::Rook:
            return "Rook";
        case Chess::PieceType::Queen:
            return "Queen";
        case Chess::PieceType::King:
            return "King";
        default:
            return "Unknown";
    }
}

DisplayManager::DisplayManager()
    : displayP1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
      displayP2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET) {}

bool DisplayManager::begin() {
    // promotionSemaphore = xSemaphoreCreateBinary();
    Wire.begin(P1_SDA, P1_SCL);
    Wire1.begin(P2_SDA, P2_SCL);

    bool success = true;

    if (!displayP1.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("Display P1 failed to initialize on Wire (I2C0)");
        success = false;
    } else {
        displayP1.clearDisplay();
        displayP1.setTextSize(1);
        displayP1.setTextColor(SSD1306_WHITE);
        displayP1.setCursor(0, 0);
        displayP1.println("OLED Ready!");
        displayP1.display();
    }

    if (!displayP2.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Display P2 failed to initialize on Wire1 (I2C1)");
        success = false;
    } else {
        displayP2.clearDisplay();
        displayP2.setTextSize(1);
        displayP2.setTextColor(SSD1306_WHITE);
        displayP2.setCursor(0, 0);
        displayP2.println("OLED Ready!");
        displayP2.display();
    }

    return success;
}

void DisplayManager::handleInput(int playerID, bool leftSpin, bool rightSpin, bool buttonPressed) {
    if (playerID < 1 || playerID > 2) return;
    if (!leftSpin && !rightSpin && !buttonPressed) return;

    PlayerUIState& s = uiState[playerID - 1];

    if (s.screen == Screen::MainMenu) {
        if (!s.timeLocked) {
            if (playerID == 1) {
                if (rightSpin) {
                    uiState[0].totalSeconds += 15;
                    if (uiState[0].totalSeconds > 3600) {
                        uiState[0].totalSeconds = 3600;
                    }
                } else if (leftSpin) {
                    uiState[0].totalSeconds -= 15;
                    if (uiState[0].totalSeconds < 15) uiState[0].totalSeconds = 15;
                }

                uiState[1].totalSeconds = uiState[0].totalSeconds;
                int m = uiState[0].totalSeconds / 60;
                int sec = uiState[0].totalSeconds % 60;
                snprintf(uiState[0].timeStr, sizeof(uiState[0].timeStr), "%02d:%02d", m, sec);
                strcpy(uiState[1].timeStr, uiState[0].timeStr);

                uiState[0].needsRedraw = true;
                uiState[1].needsRedraw = true;
            }

            if (buttonPressed && playerID == 1) {
                uiState[0].timeLocked = true;
                uiState[1].timeLocked = true;
                uiState[0].needsRedraw = true;
                uiState[1].needsRedraw = true;
            }
        } else {
            if (buttonPressed) {
                game.setState(SystemState::INIT);

                setTimeControl(uiState[0].totalSeconds);
                uiState[0].screen = Screen::PlayingMenu;
                uiState[1].screen = Screen::PlayingMenu;

                uint64_t newOccupancy = readBoardBitmap();
                game.updateInitialization(newOccupancy);
            }
        }
    } else if (s.screen == Screen::PlayingMenu) {
         if (buttonPressed) {
            if (s.menuIndex == 0) {
                s.screen = Screen::OptionsMenu;
                s.optionsIndex = 0;
                s.needsRedraw = true;
            }
        }

    } else if (s.screen == Screen::OptionsMenu) {
        constexpr uint8_t ITEMS = 4;

        if (leftSpin) {
            if (s.optionsIndex > 0) {
                s.optionsIndex--;
                s.needsRedraw = true;
            }
        } else if (rightSpin) {
            if (s.optionsIndex < ITEMS - 1) {
                s.optionsIndex++;
                s.needsRedraw = true;
            }
        } else if (buttonPressed) {
            if (s.optionsIndex == 0) {  // Return back to playing screen
                s.screen = Screen::PlayingMenu;
            } else if (s.optionsIndex == 1) {  // legal moves option
                s.legalMoves = !s.legalMoves;
                game.setSettings(playerID - 1, s.bestMoves, s.legalMoves);
            } else if (s.optionsIndex == 2) {  // best moves option
                s.bestMoves = !s.bestMoves;
                game.setSettings(playerID - 1, s.bestMoves, s.legalMoves);
            } else if (s.optionsIndex == 3) {  // reset button
                resetState(1);
                resetState(2);
                game.reset();
                uiState[0].screen = Screen::MainMenu;
                uiState[1].screen = Screen::MainMenu;
            }
            s.needsRedraw = true;
        }
    } else if (s.screen == Screen::GameOver) {
        if (buttonPressed) {
            resetState(1);
            resetState(2);
            game.reset();
            uiState[0].screen = Screen::MainMenu;
            uiState[1].screen = Screen::MainMenu;
        }
    } else if (s.screen == Screen::Promotion) {
        constexpr uint8_t ITEMS = 4;

        if (playerID != promotingPlayer) return;

        if (leftSpin && s.promotionIndex > 0) {
            s.promotionIndex--;
            s.needsRedraw = true;
        } else if (rightSpin && s.promotionIndex < ITEMS - 1) {
            s.promotionIndex++;
            s.needsRedraw = true;
        } else if (buttonPressed) {
            switch (promHL(s.promotionIndex)) {
                case PromotionHighlight::Queen:
                    promotionResult = Chess::PieceType::Queen;
                    break;
                case PromotionHighlight::Knight:
                    promotionResult = Chess::PieceType::Knight;
                    break;
                case PromotionHighlight::Bishop:
                    promotionResult = Chess::PieceType::Bishop;
                    break;
                case PromotionHighlight::Rook:
                    promotionResult = Chess::PieceType::Rook;
                    break;
            }

            s.screen = Screen::PlayingMenu;
            s.needsRedraw = true;

            // xSemaphoreGive(promotionSemaphore);
            promotionComplete = true;
        }
    }
}

void DisplayManager::updateDisplays() {
    for (int i = 0; i < 2; i++) {
        PlayerUIState& s = uiState[i];
        if (s.needsRedraw) {
            int playerID = i + 1;

            if (s.screen == Screen::MainMenu) {
                drawMainMenu(playerID, menuHL(s.menuIndex));
            } else if (s.screen == Screen::PlayingMenu) {
                drawPlayingMenu(playerID, playHL(s.menuIndex));
            } else if (s.screen == Screen::OptionsMenu) {
                drawOptionsMenu(playerID, optHL(s.optionsIndex));
            } else if (s.screen == Screen::GameOver) {
                drawGameOver(playerID, lossReason);
            } else if (s.screen == Screen::Promotion) {
                drawPromotionMenu(playerID, promHL(s.promotionIndex));
            }

            s.needsRedraw = false;
        }
    }
}

void DisplayManager::resetState(int playerID) {
    int savedTime = uiState[playerID - 1].totalSeconds;
    bool savedLegal = uiState[playerID - 1].legalMoves;
    bool savedBest = uiState[playerID - 1].bestMoves;

    uiState[playerID - 1] = PlayerUIState{};
    uiState[playerID - 1].totalSeconds = savedTime;
    uiState[playerID - 1].remainingMs = savedTime * 1000;
    uiState[playerID - 1].legalMoves = savedLegal;
    uiState[playerID - 1].bestMoves = savedBest;
    errorMsgActive = false;

    int m = savedTime / 60;
    int s = savedTime % 60;
    snprintf(uiState[playerID - 1].timeStr, sizeof(uiState[playerID - 1].timeStr), "%02d:%02d", m, s);
    uiState[playerID - 1].needsRedraw = true;
}

void DisplayManager::notifyGameEnd(const char* reason) {
    if (uiState[0].screen == Screen::GameOver) return;

    strcpy(lossReason, reason);
    uiState[0].screen = Screen::GameOver;
    uiState[1].screen = Screen::GameOver;
    uiState[0].needsRedraw = true;
    uiState[1].needsRedraw = true;
}

void DisplayManager::updateTime(int playerID) {
    if (uiState[0].screen == Screen::GameOver) {
        return;
    }

    PlayerUIState& s = uiState[playerID - 1];
    if (!s.clockRunning) {
        return;
    }

    unsigned long now = millis();
    unsigned long elapsed = now - s.lastTickMs;
    s.lastTickMs = now;

    s.remainingMs -= (int)elapsed;

    if (s.remainingMs <= 0) {
        s.remainingMs = 0;
        clearAllLEDs();
        flushLEDBuffer();
        game.setState(SystemState::GAME_OVER);
        const char* reason = (playerID == 1) ? "White timeout" : "Black timeout";
        notifyGameEnd(reason);
        return;
    }

    int secsLeft = s.remainingMs / 1000;
    if (secsLeft != s.lastDisplayedSeconds) {
        s.lastDisplayedSeconds = secsLeft;
        snprintf(s.timeStr, sizeof(s.timeStr), "%02d:%02d", secsLeft / 60, secsLeft % 60);
        s.needsRedraw = true;
    }
}

void DisplayManager::setTimeControl(int seconds) {
    for (int i = 0; i < 2; i++) {
        uiState[i].totalSeconds = seconds;
        uiState[i].remainingMs = seconds * 1000;
        uiState[i].clockRunning = false;
        uiState[i].lastTickMs = 0;
        uiState[i].lastDisplayedSeconds = -1;
        int m = seconds / 60;
        int s = seconds % 60;
        snprintf(uiState[i].timeStr, sizeof(uiState[i].timeStr), "%02d:%02d", m, s);
        uiState[i].needsRedraw = true;
    }
}

void DisplayManager::startClock(int playerID) {
    PlayerUIState& s = uiState[playerID - 1];
    s.clockRunning = true;
    s.lastTickMs = millis();
}

void DisplayManager::pauseClock(int playerID) { uiState[playerID - 1].clockRunning = false; }

void DisplayManager::resumeClock(int playerID) {
    PlayerUIState& s = uiState[playerID - 1];
    s.clockRunning = true;
    s.lastTickMs = millis();
}

void DisplayManager::startPromotion(int playerID) {
    promotingPlayer = playerID;
    promotionComplete = false;
    promotionResult = Chess::PieceType::Queen;

    uiState[playerID - 1].screen = Screen::Promotion;
    uiState[playerID - 1].promotionIndex = 0;
    uiState[playerID - 1].needsRedraw = true;
}

void DisplayManager::clearPromotionState() { promotionComplete = false; }

bool DisplayManager::isPromotionComplete() const { return promotionComplete; }

Chess::PieceType DisplayManager::getPromotionResult() const { return promotionResult; }

void DisplayManager::drawGrid(int playerID, uint64_t boardState) {
    Adafruit_SSD1306* display;
    if (playerID == 1) {
        display = &displayP1;
    } else if (playerID == 2) {
        display = &displayP2;
    }

    display->clearDisplay();

    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);

    const uint8_t CELL = 7;
    const uint8_t GAP = 1;
    const uint8_t CELL_WITH_GAP = CELL + GAP;
    const uint8_t GRID_START_X = 4;
    const uint8_t GRID_START_Y = 0;

    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 8; col++) {
            bool pressed = (boardState & (1ULL << ((row * 8) + col))) != 0;

            uint8_t x = GRID_START_X + col * CELL_WITH_GAP;
            uint8_t y = GRID_START_Y + (7 - row) * CELL_WITH_GAP;

            if (pressed) {
                display->fillRect(x, y, CELL, CELL, SSD1306_WHITE);
            } else {
                display->drawRect(x, y, CELL, CELL, SSD1306_WHITE);
            }
        }
    }

    display->display();
}

void DisplayManager::drawMainMenu(int playerID, MenuHighlight highlight) {
    Adafruit_SSD1306* display = getDisplay(playerID);
    if (!display) return;

    PlayerUIState& s = uiState[playerID - 1];

    display->clearDisplay();

    const int titleY = 14;
    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(12, titleY);
    display->print("Main Menu");

    const int labelY = titleY + 16;
    const int timeY = labelY + 12;
    const int arrowY = timeY + 8;

    if (!s.timeLocked) {
        display->setTextSize(1);

        display->setTextSize(2);
        int x = (128 - (strlen(s.timeStr) * 12)) / 2;
        display->setCursor(x, timeY);
        display->print(s.timeStr);

        if (playerID == 1) {
            display->fillTriangle(10, arrowY, 18, arrowY - 6, 18, arrowY + 6, SSD1306_WHITE);
            display->fillTriangle(118, arrowY, 110, arrowY - 6, 110, arrowY + 6, SSD1306_WHITE);
        } else {
            display->drawTriangle(10, arrowY, 18, arrowY - 6, 18, arrowY + 6, SSD1306_WHITE);
            display->drawTriangle(118, arrowY, 110, arrowY - 6, 110, arrowY + 6, SSD1306_WHITE);
        }

    } else {
        const int btnY = labelY + 12;

        display->fillRect(34, btnY, 60, 18, SSD1306_WHITE);
        display->setTextSize(1);
        display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        display->setCursor(49, btnY + 5);
        display->print("START");
    }

    display->display();
}

void DisplayManager::drawPlayingMenu(int playerID, PlayingHighlight highlight) {
    Adafruit_SSD1306* display = getDisplay(playerID);
    if (!display) return;

    PlayerUIState& s = uiState[playerID - 1];

    display->clearDisplay();

    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(38, 10);
    display->print(s.timeStr);

    display->setTextSize(1);
    display->drawLine(0, 28, 127, 28, SSD1306_WHITE);

    const uint8_t CY = 50;

   if (errorMsgActive) {
        // bool gearSelected = (highlight == PlayingHighlight::Settings);
        // if (gearSelected) {
        //     display->fillRect(0, CY - 12, 28, 24, SSD1306_WHITE);
        // }
        // drawGear(display, 14, CY, gearSelected ? SSD1306_BLACK : SSD1306_WHITE);

        drawGear(display, 14, CY, SSD1306_WHITE);
        display->drawLine(30, 30, 30, 63, SSD1306_WHITE);


        String line1 = String("Place ") + errorMsgPiece;
        String line2 = String("on ") + errorMsgSquare;
        int16_t x1 = 31 + (97 - (line1.length() * 6)) / 2;
        int16_t x2 = 31 + (97 - (line2.length() * 6)) / 2;

        display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        display->setCursor(x1, 35); 
        display->print(line1);
        display->setCursor(x2, 49); 
        display->print(line2);
    } else {
        // bool gearSelected = (highlight == PlayingHighlight::Settings);
        // if (gearSelected) {
        //     display->fillRect(64 - 14, CY - 12, 28, 24, SSD1306_WHITE);
        // }
        // drawGear(display, 64, CY, gearSelected ? SSD1306_BLACK : SSD1306_WHITE);

        drawGear(display, 64, CY, SSD1306_WHITE);
    }

    display->display();
}

void DisplayManager::drawOptionsMenu(int playerID, OptionsHighlight highlight) {
    Adafruit_SSD1306* display = getDisplay(playerID);
    if (!display) return;

    PlayerUIState& s = uiState[playerID - 1];

    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);

    display->setCursor(34, 10);
    display->print("OPTIONS");
    display->drawLine(0, 20, 127, 20, SSD1306_WHITE);

    struct {
        const char* label;
        uint8_t y;
        OptionsHighlight val;
        bool state;
    } items[] = {
        {"Back", 25, OptionsHighlight::Back, false},
        {"Legal Moves", 35, OptionsHighlight::LegalMoves, s.legalMoves},
        {"Best Moves", 45, OptionsHighlight::BestMoves, s.bestMoves},
        {"Reset", 55, OptionsHighlight::Reset, false},
    };

    for (auto& item : items) {
        bool sel = (item.val == highlight);

        if (sel) {
            display->fillRect(0, item.y - 1, 100, 10, SSD1306_WHITE);
            display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            display->setCursor(4, item.y);
            display->print("> ");
        } else {
            display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            display->setCursor(4, item.y);
            display->print("  ");
        }
        display->print(item.label);

        if (item.val != OptionsHighlight::Reset && item.val != OptionsHighlight::Back) {
            drawToggle(display, 100, item.y - 1, item.state);
        }
    }

    display->display();
}

void DisplayManager::drawGameOver(int playerID, const char* reason) {
    Adafruit_SSD1306* display = getDisplay(playerID);
    if (!display) return;

    display->clearDisplay();

    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(10, 16);
    display->print("Game Over");

    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);

    int reasonLen = strlen(reason);
    int reasonWidth = reasonLen * 6;
    int reasonX = (128 - reasonWidth) / 2;

    if (reasonX < 0) reasonX = 0;

    display->setCursor(reasonX, 38);
    display->print(reason);

    display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display->setCursor(31, 56);
    display->print("Play again?");

    display->display();
}

void DisplayManager::drawPromotionMenu(int playerID, PromotionHighlight highlight) {
    Adafruit_SSD1306* display = getDisplay(playerID);
    if (!display) return;

    PlayerUIState& s = uiState[playerID - 1];

    display->clearDisplay();

    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(38, 10);
    display->print(s.timeStr);

    display->setTextSize(1);
    display->drawLine(0, 28, 127, 28, SSD1306_WHITE);
    display->setCursor(30, 32);
    display->print("Promote to:");

    struct Item {
        PromotionHighlight val;
        const char* label;
        uint8_t cx;
    } items[] = {
        {PromotionHighlight::Queen, "Q", 16},
        {PromotionHighlight::Knight, "N", 48},
        {PromotionHighlight::Bishop, "B", 80},
        {PromotionHighlight::Rook, "R", 112},
    };

    const uint8_t CY = 52;

    for (auto& item : items) {
        bool sel = (item.val == highlight);

        if (sel) {
            display->fillRect(item.cx - 12, CY - 12, 24, 24, SSD1306_WHITE);
        }

        display->setTextSize(2);
        display->setTextColor(sel ? SSD1306_BLACK : SSD1306_WHITE, sel ? SSD1306_WHITE : SSD1306_BLACK);
        display->setCursor(item.cx - 6, CY - 8);
        display->print(item.label);
    }

    display->display();
}

void DisplayManager::showErrorMsg(Chess::Square sq, Chess::PieceType pt) {
    strncpy(errorMsgPiece, pieceTypeName(pt), sizeof(errorMsgPiece) - 1);
    errorMsgSquare[0] = 'A' + (sq % 8);
    errorMsgSquare[1] = '1' + (sq / 8);
    errorMsgSquare[2] = '\0';

    errorMsgActive = true;
    uiState[0].needsRedraw = true;
    uiState[1].needsRedraw = true;
}

void DisplayManager::clearErrorMsg() {
    errorMsgActive = false;
    uiState[0].needsRedraw = true;
    uiState[1].needsRedraw = true;
}

void DisplayManager::drawGear(Adafruit_SSD1306* d, uint8_t cx, uint8_t cy, uint16_t color) {
    d->drawCircle(cx, cy, 7, color);
    d->fillCircle(cx, cy, 4, color);
    d->fillCircle(cx, cy, 3, (color == SSD1306_WHITE) ? SSD1306_BLACK : SSD1306_WHITE);
    d->fillRect(cx - 1, cy - 10, 3, 4, color);
    d->fillRect(cx - 1, cy + 7, 3, 4, color);
    d->fillRect(cx - 10, cy - 1, 4, 3, color);
    d->fillRect(cx + 7, cy - 1, 4, 3, color);
    // Diagonal nubs
    d->fillRect(cx + 4, cy - 8, 3, 3, color);
    d->fillRect(cx - 6, cy - 8, 3, 3, color);
    d->fillRect(cx + 4, cy + 6, 3, 3, color);
    d->fillRect(cx - 6, cy + 6, 3, 3, color);
}

void DisplayManager::drawSquareIcon(Adafruit_SSD1306* d, uint8_t cx, uint8_t cy, uint16_t color) {
    d->drawRect(cx - 7, cy - 7, 15, 15, color);
    d->drawRect(cx - 4, cy - 4, 9, 9, color);
}

void DisplayManager::drawToggle(Adafruit_SSD1306* d, uint8_t x, uint8_t y, bool on) {
    d->drawRect(x, y, 26, 9, SSD1306_WHITE);
    d->drawLine(x + 13, y + 1, x + 13, y + 7, SSD1306_WHITE);

    if (on) {
        d->fillRect(x + 1, y + 1, 12, 7, SSD1306_WHITE);
        d->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        d->setCursor(x + 1, y + 1);
        d->print("ON");
    } else {
        d->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        d->setCursor(x + 14, y + 1);
        d->print("OF");
    }
}

MenuHighlight DisplayManager::menuHL(uint8_t i) {
    switch (i) {
        default:
            return MenuHighlight::None;
    }
}

PlayingHighlight DisplayManager::playHL(uint8_t i) {
    switch (i) {
        case 0:
            return PlayingHighlight::Settings;
        default:
            return PlayingHighlight::None;
    }
}

OptionsHighlight DisplayManager::optHL(uint8_t i) {
    switch (i) {
        case 0:
            return OptionsHighlight::Back;
        case 1:
            return OptionsHighlight::LegalMoves;
        case 2:
            return OptionsHighlight::BestMoves;
        case 3:
            return OptionsHighlight::Reset;
        default:
            return OptionsHighlight::None;
    }
}

PromotionHighlight DisplayManager::promHL(uint8_t i) {
    switch (i) {
        case 0:
            return PromotionHighlight::Queen;
        case 1:
            return PromotionHighlight::Knight;
        case 2:
            return PromotionHighlight::Bishop;
        case 3:
            return PromotionHighlight::Rook;
        default:
            return PromotionHighlight::Queen;
    }
}

Adafruit_SSD1306* DisplayManager::getDisplay(int playerID) {
    if (playerID == 1)
        return &displayP1;
    else if (playerID == 2)
        return &displayP2;
    return nullptr;
}