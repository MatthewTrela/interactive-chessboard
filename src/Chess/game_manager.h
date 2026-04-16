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
    GameManager();
    void init();
    void updateBoard(uint64_t newBoard);
    PlayerSettings& getSettings(uint8_t player);  // player: 0 or 1
    Chess::Board& getBoard();

private:
    SystemState currentState;
    Chess::Board currentBoard;
    PlayerSettings players[2];
};

extern GameManager game;