#pragma once

#include <Arduino.h>

#include "chess.hpp"

enum class SystemState { MAIN_MENU, PLAYING, ERROR_RECOVERY };

struct PlayerSettings {
    bool showBestMove;
    bool showLegalMoves;
};

class GameManager {
   public:
    // constructor
    GameManager();

    // initialize game to starting state
    void init();

    // new board state
    void updateBoard(uint64_t newBoard);

    // change turns
    PlayerSettings& getSettings(uint8_t player);  // player: 0 or 1

    // set settings
    void setSettings(uint8_t playerNum, bool showBestMove, bool showLegalMoves);

    // returns the board
    Chess::Board& getBoard();

   private:
    SystemState currentState;
    Chess::Board currentBoard;
    PlayerSettings players[2];
};

extern GameManager game;