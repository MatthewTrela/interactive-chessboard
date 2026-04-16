#include "game_manager.h"

GameManager game;

GameManager::GameManager() : currentState(SystemState::MAIN_MENU), currentBoard() {}

void GameManager::init() {
    // Initilize attack tables
    static bool attacksInitialized = false;
    if (!attacksInitialized) {
        Chess::Attacks::init();
        attacksInitialized = true;
    }
    // Set chessboard to starting position
    currentBoard.reset();
    // Set initial user settings
    players[0] = PlayerSettings{false, false};
    players[1] = PlayerSettings{false, false};
}

PlayerSettings& GameManager::getSettings(uint8_t player) { return players[player]; }

void GameManager::setSettings(uint8_t playerNum, bool showBestMove, bool showLegalMoves) {
    // out of bounds checking
    if (playerNum >= 2 || playerNum < 0) return;

    players[playerNum].showBestMove = showBestMove;
    players[playerNum].showLegalMoves = showLegalMoves;
}

Chess::Board& GameManager::getBoard() { return currentBoard; }