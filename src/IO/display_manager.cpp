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

    setTimeControl(60 * 10);

    return success;
}

void DisplayManager::handleInput(int playerID, bool leftSpin, bool rightSpin, bool buttonPressed) {
    if (playerID < 1 || playerID > 2) return;
    if (!leftSpin && !rightSpin && !buttonPressed) return;

    PlayerUIState& s = uiState[playerID - 1];

    if (s.screen == Screen::MainMenu) {
        constexpr uint8_t ITEMS = 3;

        if (leftSpin) {
            s.menuIndex = (s.menuIndex + ITEMS - 1) % ITEMS;
            s.needsRedraw = true;
        } else if (rightSpin) {
            s.menuIndex = (s.menuIndex + 1) % ITEMS;
            s.needsRedraw = true;
        } else if (buttonPressed) {
            if (s.menuIndex == 0) {
                s.screen = Screen::OptionsMenu;
                s.optionsIndex = 0;
                s.needsRedraw = true;
            }
            // TODO: Undo (index 1), Square (index 2)
        }

    } else if (s.screen == Screen::OptionsMenu) {
        constexpr uint8_t ITEMS = 3;

        if (leftSpin) {
            if (s.optionsIndex == 0) {
                s.screen = Screen::MainMenu;
                s.needsRedraw = true;
            } else {
                s.optionsIndex--;
                s.needsRedraw = true;
            }
        } else if (rightSpin) {
            if (s.optionsIndex < ITEMS - 1) {
                s.optionsIndex++;
                s.needsRedraw = true;
            }
        } else if (buttonPressed) {
            if (s.optionsIndex == 0) {  // legal moves option
                s.legalMoves = !s.legalMoves;
                game.setSettings(playerID - 1, s.bestMoves, s.legalMoves);
            } else if (s.optionsIndex == 1) {  // best moves option
                s.bestMoves = !s.bestMoves;
                game.setSettings(playerID - 1, s.bestMoves, s.legalMoves);
            } else if (s.optionsIndex == 2) {  // reset button
                resetState(1);
                resetState(2);
                game.init();
            }
            s.needsRedraw = true;
        }
    } else if (s.screen == Screen::GameOver) {
        if (buttonPressed) {
            resetState(1);
            resetState(2);
            game.init();
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
            } else if (s.screen == Screen::OptionsMenu) {
                drawOptionsMenu(playerID, optHL(s.optionsIndex));
            } else if (s.screen == Screen::GameOver) {
                drawGameOver(playerID, lossReason);
            }

            s.needsRedraw = false;
        }
    }
}

void DisplayManager::resetState(int playerID) {
    // 1. Save the persistent variables
    int savedTime = uiState[playerID - 1].totalSeconds;
    bool savedLegal = uiState[playerID - 1].legalMoves;
    bool savedBest = uiState[playerID - 1].bestMoves;

    uiState[playerID - 1] = PlayerUIState{};
    uiState[playerID - 1].totalSeconds = savedTime;
    uiState[playerID - 1].legalMoves = savedLegal;
    uiState[playerID - 1].bestMoves = savedBest;
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

void DisplayManager::printMessage(int playerID, int line, const String& message, bool instantUpdate) {
    Adafruit_SSD1306* targetDisplay = nullptr;

    if (playerID == 1) {
        targetDisplay = &displayP1;
    } else if (playerID == 2) {
        targetDisplay = &displayP2;
    } else {
        return;
    }

    int yPos = line * 10;

    targetDisplay->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    targetDisplay->setTextSize(1);
    targetDisplay->setCursor(0, yPos);
    targetDisplay->print(message);

    if (instantUpdate) {
        targetDisplay->display();
    }
}

void DisplayManager::updateTime(int playerID) {
    if (uiState[0].screen == Screen::GameOver) return;

    PlayerUIState& s = uiState[playerID - 1];
    if (s.startTime == 0) s.startTime = millis();

    unsigned long timeChange = millis() - s.startTime;
    int timeLeftSecs = s.totalSeconds - (int)(timeChange / 1000);

    if (timeLeftSecs <= 0) {
        clearAllLEDs();
        flushLEDBuffer();
        game.setState(SystemState::GAME_OVER);
        const char* reason = (playerID == 1) ? "White timeout" : "Black timeout";
        notifyGameEnd(reason);
        return;
    }

    // Generate time string for OLED
    if (timeLeftSecs != s.lastSeconds) {
        s.lastSeconds = timeLeftSecs;
        int minutes = timeLeftSecs / 60;
        int seconds = timeLeftSecs % 60;
        snprintf(s.timeStr, sizeof(s.timeStr), "%02d:%02d", minutes, seconds);
        s.needsRedraw = true;
    }
}

void DisplayManager::setTimeControl(int seconds) {
    for (int i = 0; i < 2; i++) {
        uiState[i].totalSeconds = seconds;
        int m = seconds / 60;
        int s = seconds % 60;
        snprintf(uiState[i].timeStr, sizeof(uiState[i].timeStr), "%02d:%02d", m, s);
        uiState[i].needsRedraw = true;
    }
}

void DisplayManager::startClock(int playerID) {
    PlayerUIState& s = uiState[playerID - 1];
    s.startTime = millis();
}

void DisplayManager::drawGrid(int playerID, uint64_t boardState) {
    Adafruit_SSD1306* display;
    if (playerID == 1) {
        display = &displayP1;
    } else if (playerID == 2) {
        display = &displayP2;
    }

    display->clearDisplay();

    // Draw title
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);

    // Draw 8x8 grid
    // 128 wide, leave margins → 14px per square + 1px gap = 15px
    const uint8_t CELL = 7;
    const uint8_t GAP = 1;
    const uint8_t CELL_WITH_GAP = CELL + GAP;
    const uint8_t GRID_START_X = 4;
    const uint8_t GRID_START_Y = 0;

    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 8; col++) {
            bool pressed = (boardState & (1ULL << ((row * 8) + col))) != 0;

            uint8_t x = GRID_START_X + col * CELL_WITH_GAP;
            uint8_t y = GRID_START_Y + (7 - row) * CELL_WITH_GAP;  // flip Y so row 0 = top

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

    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(38, 2);
    display->print(s.timeStr);
    display->setTextSize(1);
    display->drawLine(0, 20, 127, 20, SSD1306_WHITE);

    const uint8_t CY = 42;

    struct Item {
        MenuHighlight val;
        uint8_t cx;
    } items[] = {
        {MenuHighlight::Settings, 21},
        {MenuHighlight::Undo, 64},
        {MenuHighlight::Square, 107},
    };

    for (auto& item : items) {
        bool sel = (item.val == highlight);
        uint16_t fg = sel ? SSD1306_BLACK : SSD1306_WHITE;

        if (sel) {
            display->fillRect(item.cx - 14, CY - 12, 28, 24, SSD1306_WHITE);
        }

        if (item.val == MenuHighlight::Settings) {
            drawGear(display, item.cx, CY, fg);

        } else if (item.val == MenuHighlight::Undo) {
            display->setTextColor(fg, sel ? SSD1306_WHITE : SSD1306_BLACK);
            display->setCursor(item.cx - 12, CY - 4);
            display->print("UNDO");

        } else if (item.val == MenuHighlight::Square) {
            drawSquareIcon(display, item.cx, CY, fg);
        }
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
    display->setCursor(34, 2);
    display->print("OPTIONS");
    display->drawLine(0, 12, 127, 12, SSD1306_WHITE);

    struct {
        const char* label;
        uint8_t y;
        OptionsHighlight val;
        bool state;
    } items[] = {
        {"Legal Moves", 22, OptionsHighlight::LegalMoves, s.legalMoves},
        {"Best Moves", 34, OptionsHighlight::BestMoves, s.bestMoves},
        {"Reset", 46, OptionsHighlight::Reset, false},
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

        if (item.val != OptionsHighlight::Reset) {
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
    display->setCursor(10, 8);
    display->print("Game Over");

    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);

    int reasonLen = strlen(reason);
    int reasonWidth = reasonLen * 6;
    int reasonX = (128 - reasonWidth) / 2;

    if (reasonX < 0) reasonX = 0;

    display->setCursor(reasonX, 30);
    display->print(reason);

    display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display->setCursor(31, 48);  // Moved down to Y=48
    display->print("Play again?");

    display->display();
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
        d->print("OFF");
    }
}

MenuHighlight DisplayManager::menuHL(uint8_t i) {
    switch (i) {
        case 0:
            return MenuHighlight::Settings;
        case 1:
            return MenuHighlight::Undo;
        case 2:
            return MenuHighlight::Square;
        default:
            return MenuHighlight::None;
    }
}

OptionsHighlight DisplayManager::optHL(uint8_t i) {
    switch (i) {
        case 0:
            return OptionsHighlight::LegalMoves;
        case 1:
            return OptionsHighlight::BestMoves;
        case 2:
            return OptionsHighlight::Reset;
        default:
            return OptionsHighlight::None;
    }
}

Adafruit_SSD1306* DisplayManager::getDisplay(int playerID) {
    if (playerID == 1)
        return &displayP1;
    else if (playerID == 2)
        return &displayP2;
    return nullptr;
}