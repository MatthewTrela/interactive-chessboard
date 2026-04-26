#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "Chess/chess.hpp"
#include "global.h"

enum class MenuHighlight { None };
enum class Screen { MainMenu, PlayingMenu, OptionsMenu, GameOver, Promotion };
enum class PlayingHighlight { None, Settings};
enum class OptionsHighlight { None, Back, LegalMoves, BestMoves, Reset };
enum class PromotionHighlight { Queen, Knight, Bishop, Rook };

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
    uint8_t promotionIndex = 0;
};

class DisplayManager {
   public:
    DisplayManager();

    // Conect displays to I2C bus
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
    void drawPromotionMenu(int playerID, PromotionHighlight highlight);

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

    // Show error message to help user fix error state
    void showErrorMsg(Chess::Square sq, Chess::PieceType pt);

    // Clear error message
    void clearErrorMsg();

    const PlayerUIState& getState(int playerID) const { return uiState[playerID - 1]; }

    void startPromotion(int playerID);
    void clearPromotionState();
    bool isPromotionComplete() const;
    Chess::PieceType getPromotionResult() const;

   private:
    // OLED objects
    Adafruit_SSD1306 displayP1;
    Adafruit_SSD1306 displayP2;

    // State information for each user
    PlayerUIState uiState[2];

    // Reason for loss on game over
    char lossReason[64] = {};

    // Private promotion variables
    Chess::PieceType promotionResult = Chess::PieceType::Queen;
    int promotingPlayer = 0;
    bool promotionComplete;

    // Private error message variables
    bool errorMsgActive = false;
    char errorMsgPiece[8]  = {};
    char errorMsgSquare[3] = {};

    // Drawing symbol helpers
    static void drawGear(Adafruit_SSD1306* d, uint8_t cx, uint8_t cy, uint16_t color);
    static void drawSquareIcon(Adafruit_SSD1306* d, uint8_t cx, uint8_t cy, uint16_t color);
    static void drawToggle(Adafruit_SSD1306* d, uint8_t x, uint8_t y, bool on);

    // Highlight index to enum helpers
    static PromotionHighlight promHL(uint8_t index);
    static MenuHighlight menuHL(uint8_t index);
    static OptionsHighlight optHL(uint8_t index);
    static PlayingHighlight playHL(uint8_t index);
    Adafruit_SSD1306* getDisplay(int playerID);
};