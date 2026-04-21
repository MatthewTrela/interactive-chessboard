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

void DisplayManager::updateDisplays() {
    if (stateP1.needsRedraw) {
        displayP1.clearDisplay();
        // Switch statement based on stateP1.currentState goes here
        displayP1.display();
        stateP1.needsRedraw = false;
    }

    if (stateP2.needsRedraw) {
        displayP2.clearDisplay();
        // Switch statement based on stateP2.currentState goes here
        displayP2.display();
        stateP2.needsRedraw = false;
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

void DisplayManager::showPickedUpPiece(Chess::PieceType piece, Chess::ChessColor color) {
    int playerID = (color == Chess::ChessColor::White) ? 1 : 2;
    String colorStr = (color == Chess::ChessColor::White) ? "White" : "Black";

    printMessage(playerID, 0, "Picked up:      ", false);
    printMessage(playerID, 1, colorStr + " " + pieceTypeName(piece), true);
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