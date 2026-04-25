#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "Chess/chess.hpp"
#include "global.h"

enum class MenuHighlight {None};
enum class Screen {MainMenu, PlayingMenu, OptionsMenu, GameOver};
enum class PlayingHighlight { None, Settings, Undo, Square};
enum class OptionsHighlight{ None, LegalMoves, BestMoves, Reset};

struct PlayerUIState {
    Screen  screen = Screen::MainMenu;
    uint8_t menuIndex = 0;
    uint8_t optionsIndex = 0;
    unsigned long startTime = 0;
    int totalSeconds = 600; 
    bool legalMoves = true;
    bool bestMoves = true;
    bool needsRedraw = false;
    char timeStr[6] = "10:00"; 
    int lastSeconds = -1;
    bool timeLocked = false;
};

class DisplayManager {
public:
    DisplayManager();

    bool begin();
    void updateDisplays();
    void handleInput(int playerID, bool leftSpin, bool rightSpin, bool buttonPressed);
    void drawGrid(int playerID, uint64_t boardState);
    void drawMainMenu(int playerID, MenuHighlight highlight);
    void drawPlayingMenu(int playerID, PlayingHighlight highlight);
    void drawOptionsMenu(int playerID, OptionsHighlight highlight);
    void drawGameOver(int playerID, const char* reason);
    void updateTime(int playerID);
    void startClock(int playerID);
    void setTimeControl(int seconds);
    void resetState(int playerID);
    void notifyGameEnd(const char* reason);

    const PlayerUIState& getState(int playerID) const {
        return uiState[playerID - 1];
    }

private:
    Adafruit_SSD1306 displayP1;
    Adafruit_SSD1306 displayP2;

    PlayerUIState uiState[2];
    char lossReason[64] = {};

    static void drawGear (Adafruit_SSD1306* d, uint8_t cx, uint8_t cy, uint16_t color);
    static void drawSquareIcon(Adafruit_SSD1306* d, uint8_t cx, uint8_t cy, uint16_t color);
    static void drawToggle(Adafruit_SSD1306* d, uint8_t x, uint8_t y, bool on);
    static MenuHighlight menuHL(uint8_t index);
    static OptionsHighlight optHL (uint8_t index);
    static PlayingHighlight playHL(uint8_t index);
    Adafruit_SSD1306* getDisplay(int playerID);
};