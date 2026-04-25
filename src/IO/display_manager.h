#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "Chess/chess.hpp"
#include "global.h"

enum class MenuHighlight {None};
enum class Screen {MainMenu, PlayingMenu, OptionsMenu, GameOver};
enum class PlayingHighlight { None, Settings, Undo, Square};
enum class OptionsHighlight{ None, Back, LegalMoves, BestMoves, Reset};

struct PlayerUIState {
    Screen screen = Screen::MainMenu;
    uint8_t menuIndex = 0;
    uint8_t optionsIndex = 0;

    // Timer tracking
    int remainingMs = 600000;
    unsigned long lastTickMs = 0;
    bool clockRunning = false;

    // Config/display
    int totalSeconds = 600;
    bool legalMoves = true;
    bool bestMoves = true;
    bool needsRedraw = false;
    char timeStr[6] = "10:00";
    int lastDisplayedSeconds = -1;
    bool timeLocked = false;
};

class DisplayManager {
public:
    DisplayManager();

    //Conect displays to I2C bus
    bool begin();

    // Where all drawing methods are called
    // Called periodically by task every 100ms
    void updateDisplays();

    // Called by task when an encoder interrupt is detected
    // Translated encoder input to menu action
    void handleInput(int playerID, bool leftSpin, bool rightSpin, bool buttonPressed);

    // Displays the reed switch detection for every square
    // Used for debugging
    void drawGrid(int playerID, uint64_t boardState);

    // Menu drawing methods that draw the corresponding screen with whatever is highlighted
    void drawMainMenu(int playerID, MenuHighlight highlight);
    void drawPlayingMenu(int playerID, PlayingHighlight highlight);
    void drawOptionsMenu(int playerID, OptionsHighlight highlight);
    void drawGameOver(int playerID, const char* reason);

    // Timer methods for handeling clock
    // ------------------------------------------------------------------------------
    // Updates the time on the OLED screen called periodically when in playing state
    void updateTime(int playerID);

    // Called when a player makes thier first move
    void startClock(int playerID);

    // Set the time each player gets to make all moves
    void setTimeControl(int seconds);

    // Pauses clock when gameboard is in error state 
    void pauseClock(int playerID);

    // Resumes clock when gameboard leaves error state
    void resumeClock(int playerID);

    // Return back to main menu screen and reset some local variables like time
    void resetState(int playerID);

    // The game task will tell us when the game ends due to checkmate or stalemate
    // Transition to gameover screen with reason for loss
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