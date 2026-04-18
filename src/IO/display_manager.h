#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "Chess/chess.hpp"
#include "global.h"

enum class MenuState {
    BOOT_SCREEN,
    GAME_BOARD,
    MAIN_MENU,
    SETTINGS_MENU,
    MESSAGE_POPUP
};

struct PlayerUIState {
    MenuState currentState = MenuState::BOOT_SCREEN;
    int menuCursorIndex = 0;
    int scrollOffset = 0;
    String currentMessage = "";
    bool needsRedraw = true;
};

class DisplayManager {
public:
    DisplayManager();

    bool begin();
    void updateDisplays();
    void printMessage(int playerID, int line, const String& message, bool instantUpdate = false);
    void showPickedUpPiece(Chess::PieceType piece, Chess::ChessColor color);


private:
    Adafruit_SSD1306 displayP1;
    Adafruit_SSD1306 displayP2;

    PlayerUIState stateP1;
    PlayerUIState stateP2;

};