#include "display_manager.h"
#include "IO/io_expander.h"

static const char* pieceTypeName(Chess::PieceType pt) {
    switch (pt) {
        case Chess::PieceType::Pawn:   return "Pawn";
        case Chess::PieceType::Knight: return "Knight";
        case Chess::PieceType::Bishop: return "Bishop";
        case Chess::PieceType::Rook:   return "Rook";
        case Chess::PieceType::Queen:  return "Queen";
        case Chess::PieceType::King:   return "King";
        default:                       return "Unknown";
    }
}

DisplayManager::DisplayManager() 
    : displayP1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
      displayP2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET)
{}

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

    return success;
}

void DisplayManager::handleInput(int playerID, bool leftSpin, bool rightSpin, bool buttonPressed) {
    if (playerID < 1 || playerID > 2) return;
    if (!leftSpin && !rightSpin && !buttonPressed) return;

    PlayerUIState& s = uiState[playerID - 1];

    if (s.screen == Screen::MainMenu) {
        constexpr uint8_t ITEMS = 4;

        if (leftSpin) {
            s.menuIndex = (s.menuIndex + ITEMS - 1) % ITEMS;
            drawMainMenu(playerID, menuHL(s.menuIndex));

        } else if (rightSpin) {
            s.menuIndex = (s.menuIndex + 1) % ITEMS;
            drawMainMenu(playerID, menuHL(s.menuIndex));

        } else if (buttonPressed) {
            if (s.menuIndex == 0) {                 // Arrow → enter Options
                s.screen       = Screen::OptionsMenu;
                s.optionsIndex = 0;
                drawOptionsMenu(playerID, optHL(0));
            }
            // TODO: handle Left / Right / Button actions (indices 1-3)
        }

    } else if (s.screen == Screen::OptionsMenu) {
        constexpr uint8_t ITEMS = 2;

        if (leftSpin) {
            if (s.optionsIndex == 0) {
                s.screen = Screen::MainMenu;
                drawMainMenu(playerID, menuHL(s.menuIndex));
            } else {
                s.optionsIndex--;
                drawOptionsMenu(playerID, optHL(s.optionsIndex));
            }

        } else if (rightSpin) {
            if (s.optionsIndex < ITEMS - 1) {
                s.optionsIndex++;
                drawOptionsMenu(playerID, optHL(s.optionsIndex));
            }

        } else if (buttonPressed) {
            s.legalMoves = (s.optionsIndex == 0);
            s.bestMoves  = (s.optionsIndex == 1);
            s.screen     = Screen::MainMenu;
            drawMainMenu(playerID, menuHL(s.menuIndex));
        }
    }
}

void DisplayManager::updateDisplays() {
    if (uiState[0].needsRedraw) {
        displayP1.clearDisplay();
        displayP1.display();
        uiState[0].needsRedraw = false;
    }

    if (uiState[1].needsRedraw) {
        displayP2.clearDisplay();
        displayP2.display();
        uiState[1].needsRedraw = false;
    }
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

void DisplayManager::drawGrid(int playerID, uint64_t boardState) {
    Adafruit_SSD1306 *display;
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
    Adafruit_SSD1306* display = nullptr;
    if      (playerID == 1) display = &displayP1;
    else if (playerID == 2) display = &displayP2;
    else return;

    display->clearDisplay();
    display->setTextSize(1);

    // Clock for Moves
    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(38, 2);
    display->print("10:00");
    display->setTextSize(1);

    display->drawLine(0, 20, 127, 20, SSD1306_WHITE);

    const uint8_t A_TIP_X  = 3;
    const uint8_t A_TIP_Y  = 41;
    const uint8_t A_TAIL_X = 12;
    const uint8_t A_TAIL_T = 25;
    const uint8_t A_TAIL_B = 57;

    const bool arrowSelected = (highlight == MenuHighlight::Arrow);
    const uint16_t arrowFg   = arrowSelected ? SSD1306_BLACK : SSD1306_WHITE;

    if (arrowSelected) {
        display->fillRect(0, A_TAIL_T - 1, A_TAIL_X + 3,(A_TAIL_B - A_TAIL_T) + 3, SSD1306_WHITE);
    }

    display->drawLine(A_TAIL_X,     A_TAIL_T, A_TIP_X,     A_TIP_Y,  arrowFg); // top leg
    display->drawLine(A_TIP_X,      A_TIP_Y,  A_TAIL_X,     A_TAIL_B, arrowFg); // bottom leg
    display->drawLine(A_TAIL_X + 1, A_TAIL_T, A_TIP_X + 1, A_TIP_Y,  arrowFg);
    display->drawLine(A_TIP_X + 1,  A_TIP_Y,  A_TAIL_X + 1, A_TAIL_B, arrowFg);

    struct { const char* label; uint8_t x; MenuHighlight val; } items[] = {
        { "LEFT",  18, MenuHighlight::Left   },
        { "RIGHT", 55, MenuHighlight::Right  },
        { "BTN",   96, MenuHighlight::Button },
    };

    const uint8_t Y = 37;

    for (auto& item : items) {
        if (item.val == highlight) {
            uint8_t w = strlen(item.label) * 6;
            display->fillRect(item.x - 1, Y - 1, w + 2, 10, SSD1306_WHITE);
            display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
            display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        }
        display->setCursor(item.x, Y);
        display->print(item.label);
    }

    display->display();
}

void DisplayManager::drawOptionsMenu(int playerID, OptionsHighlight highlight) {
    Adafruit_SSD1306* display = nullptr;
    if      (playerID == 1) display = &displayP1;
    else if (playerID == 2) display = &displayP2;
    else return;

    display->clearDisplay();
    display->setTextSize(1);

    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(34, 2);
    display->print("OPTIONS");
    display->drawLine(0, 12, 127, 12, SSD1306_WHITE);

    struct {
        const char*      label;
        uint8_t          y;
        OptionsHighlight val;
    } items[] = {
        { "Legal Moves", 22, OptionsHighlight::LegalMoves },
        { "Best Moves",  40, OptionsHighlight::BestMoves  },
    };

    for (auto& item : items) {
        if (item.val == highlight) {
            display->fillRect(0, item.y - 1, 128, 10, SSD1306_WHITE);
            display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            display->setCursor(4, item.y);
            display->print("> ");
        } else {
            display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            display->setCursor(4, item.y);
            display->print("  ");
        }
        display->print(item.label);
    }

    display->display();
}

MenuHighlight DisplayManager::menuHL(uint8_t i) {
    switch (i) {
        case 0:  return MenuHighlight::Arrow;
        case 1:  return MenuHighlight::Left;
        case 2:  return MenuHighlight::Right;
        case 3:  return MenuHighlight::Button;
        default: return MenuHighlight::None;
    }
}

OptionsHighlight DisplayManager::optHL(uint8_t i) {
    return (i == 0) ? OptionsHighlight::LegalMoves
                    : OptionsHighlight::BestMoves;
}

Adafruit_SSD1306* DisplayManager::getDisplay(int playerID) {
    if      (playerID == 1) return &displayP1;
    else if (playerID == 2) return &displayP2;
    return nullptr;
}