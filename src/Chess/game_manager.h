#pragma once

#include <Arduino.h>

#include "chess.hpp"

enum class SystemState { MAIN_MENU, PLAYING, ERROR_RECOVERY, GAME_OVER };

struct PlayerSettings {
    bool showBestMove;
    bool showLegalMoves;
};

class GameManager {
   private:
    SystemState currentState;
    Chess::Board currentBoard;  // valid chess position
    PlayerSettings players[2];

    // track partial moves
    Chess::Square pickupSquare;
    Chess::PieceType pickedUpPiece;
    Chess::ChessColor pickedUpColor;
    bool hasPickedUpPiece;

    // sensor state tracking
    Chess::Bitboard sensorOccupancy;  // current physical board state

   public:
    // constructor
    GameManager();

    // initialize game to starting state
    void init();

    // new board state
    void updateBoard(uint64_t newBoard);

    // ------------ helper methods for updating board state
    void updateSensorOccupancy(Chess::Square square, bool occupied);

    void handlePiecePickup(Chess::Square square);

    void handlePiecePlacement(Chess::Square square);

    void checkGameEndConditions();

    // change turns
    PlayerSettings& getSettings(uint8_t player);  // player: 0 or 1

    // set settings
    void setSettings(uint8_t playerNum, bool showBestMove, bool showLegalMoves);

    // returns the board
    Chess::Board& getBoard();
};

extern GameManager game;