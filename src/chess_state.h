#ifndef CHESS_STATE_H
#define CHESS_STATE_H

#include <Arduino.h>
#include <stdint.h>

enum class MoveResult {
    OK,          // Legal move, board updated
    ILLEGAL,     // Move not legal in current position
    AMBIGUOUS,   // Piece lifted but destination not yet known
    WRONG_TURN,  // Right piece, wrong color to move
    ERROR        // Bad input
};

enum class SquareEvent {
    PIECE_LIFTED,  // Square went from occupied to empty
    PIECE_PLACED,  // Square went from empty to occupied
};

// Call once in setup(), after initGlobals()
void initChessState();

// Call this whenever a square changes occupancy
MoveResult onSquareEvent(uint8_t row, uint8_t col, SquareEvent event);

// Returns true if the given square is occupied per the chess model
bool isOccupiedByModel(uint8_t row, uint8_t col);

// Returns a FEN string of the current position (for serial debug / engine)
// Returns nullptr if board is in mid-move state
const char* getCurrentFEN();

// Reset to starting position
void resetToStartPosition();

#endif
