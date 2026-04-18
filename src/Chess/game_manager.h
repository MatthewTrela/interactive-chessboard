#pragma once

#include <Arduino.h>

#include "chess.hpp"

enum class SystemState { MAIN_MENU, PLAYING, ERROR_RECOVERY, GAME_OVER };

struct PlayerSettings {
    bool showBestMove;
    bool showLegalMoves;
};

/// Tracks what physical step of a move sequence the player is currently in.
///
/// Normal move:
///   IDLE → ATTACKER_LIFTED → (place on destination) → IDLE
///
/// Capture move (physical sequence: pick up attacker, remove captured piece,
/// place attacker on destination square):
///   IDLE → ATTACKER_LIFTED → CAPTURED_REMOVED → (place on destination) → IDLE
///
/// The player must always lift their own piece first. Once an attacker is
/// lifted, lifting the opponent piece on the target square advances the phase
/// to CAPTURED_REMOVED rather than going to error.
enum class MovePhase {
    IDLE,              // No piece in hand; waiting for the moving side to act.
    ATTACKER_LIFTED,   // Moving-side piece is in hand; to-square not yet known.
    CAPTURED_REMOVED,  // Attacker in hand AND captured piece removed from board;
                       // next placement must be on capturedSquare.
};

class GameManager {
   public:
    // constructor
    GameManager();

    // initialize game to starting state
    void init();

    /// Called whenever a hall-effect / reed switch changes state.
    /// @param newBoard  64-bit occupancy bitmask reflecting the current
    ///                  physical board (1 = piece present, LSB = A1).
    void updateBoard(uint64_t newBoard);

    PlayerSettings& getSettings(uint8_t player);  // player: 0 or 1

    void setSettings(uint8_t playerNum, bool showBestMove, bool showLegalMoves);

    Chess::Board& getBoard();

   private:
    // game state
    SystemState currentState;
    Chess::Board currentBoard;  // valid chess position
    PlayerSettings players[2];

    // sensor state tracking
    Chess::Bitboard sensorOccupancy;

    // move phase state machine
    MovePhase movePhase;
    Chess::Square attackingSquare;  // where the moving piece came from
    Chess::Move pendingMove;        // fully-resolved legal move (set once both
                                    // from- and to-squares are known)
    // Set when movePhase == CAPTURED_REMOVED
    Chess::Square capturedSquare;  // square the captured piece was on

    // internal helper methods
    void updateSensorOccupancy(Chess::Square sq, bool occupied);
    void handlePiecePickup(Chess::Square sq);
    void handlePiecePlacement(Chess::Square sq);
    /// Searches the legal-move list for a move from `from` to `to`.
    /// Returns true and sets `out` on success.
    bool findLegalMove(Chess::Square from, Chess::Square to, Chess::Move& out) const;
    /// Returns true if any legal move captures a piece on `targetSq`.
    bool squareIsCaptureTarget(Chess::Square targetSq) const;
    /// Validates, applies `move` to the board, resets move-phase, and checks
    /// for game-end conditions. Sets ERROR_RECOVERY on failure.
    void executeMove(Chess::Move move);
    /// Resets all move-phase variables to IDLE without touching board state.
    void resetMovePhase();
    void checkGameEndConditions();
};

extern GameManager game;