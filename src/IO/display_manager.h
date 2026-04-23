#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "Chess/chess.hpp"
#include "global.h"

enum class Screen { MainMenu, OptionsMenu };
enum class MenuHighlight { None, Arrow, Left, Right, Button };
enum class OptionsHighlight{ None, LegalMoves, BestMoves };

struct PlayerUIState {
    Screen  screen       = Screen::MainMenu;
    uint8_t menuIndex    = 0;
    uint8_t optionsIndex = 0;
    bool    legalMoves   = false;
    bool    bestMoves    = false;
    bool    needsRedraw  = false;
};

class DisplayManager {
public:
    DisplayManager();

    bool begin();
    void updateDisplays();
    void handleInput(int playerID, bool leftSpin, bool rightSpin, bool buttonPressed);
    void printMessage(int playerID, int line, const String& message, bool instantUpdate = false);
    void drawGrid(int playerID, uint64_t boardState);
    void drawMainMenu(int playerID, MenuHighlight highlight);
    void drawOptionsMenu(int playerID, OptionsHighlight highlight);

    const PlayerUIState& getState(int playerID) const {
        return uiState[playerID - 1];
    }

private:
    Adafruit_SSD1306 displayP1;
    Adafruit_SSD1306 displayP2;

    PlayerUIState uiState[2];

    static MenuHighlight    menuHL(uint8_t index);
    static OptionsHighlight optHL (uint8_t index);
    Adafruit_SSD1306* getDisplay(int playerID);
};