#pragma once

#include <Arduino.h>

#include "chess.hpp"

enum class SystemState { MAIN_MENU, INIT, PLAYING, ERROR_RECOVERY, GAME_OVER, PROMOTION_SELECTION, IDLE };

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
    IDLE,                  // No piece in hand; waiting for the moving side to act.
    ATTACKER_LIFTED,       // own piece lifted
    CAPTURED_REMOVED,      // opponent piece lifted, attacker in hand: normal captures and en passant
    EP_ATTACKER_PLACED,    // en passant: attacker already placed on destination, waiting for captured pawn removal
    CASTLING_ROOK_LIFTED,  // castling initiated by lifting the rook first (king still on board)
    CASTLING_KING_LIFTED,
    CASTLING_BOTH_LIFTED,  // King and correct rook both in hand
    CASTLING_ROOK_PLACED,
    CASTLING_KING_PLACED,
};

class GameManager {
   public:
    GameManager();

    // reset to starting position
    void init();

    // Turn off all LEDs, reset board, and enter main menu state
    void reset();

    void updateInitialization(uint64_t sensorState);

    /// Called whenever a hall-effect / reed switch changes state.
    /// @param newBoard  64-bit occupancy bitmask reflecting the current
    ///                  physical board (1 = piece present, LSB = A1).
    void updateBoard(uint64_t newBoard);

    PlayerSettings& getSettings(uint8_t player);  // player: 0 or 1

    void setSettings(uint8_t playerNum, bool showBestMove, bool showLegalMoves);

    Chess::Board& getBoard();
    SystemState getState();
    void setState(SystemState state) { currentState = state; }
    bool isDebouncing() const { return debounceStartTime != 0; }
    Chess::PieceType getPromotionChoice() const { return promotionChoice; }
    void setPromotionChoice(Chess::PieceType pt) { promotionChoice = pt; }
    void finalizePromotion();

   private:
    // game state
    SystemState currentState;
    Chess::Board currentBoard;  // valid chess position
    PlayerSettings players[2];

    // sensor state tracking
    Chess::Bitboard sensorOccupancy;
    Chess::Bitboard lastDebouncedBoard;
    unsigned long debounceStartTime;

    // move phase state machine
    MovePhase movePhase;
    Chess::Square attackingSquare;  // where the moving piece came from
    Chess::Move pendingMove;        // fully-resolved legal move (set once both
                                    // from- and to-squares are known)
    // Set when movePhase == CAPTURED_REMOVED
    Chess::Square capturedSquare;  // square the captured piece was on
    // Set when movePhase >= CASTLING_BOTH_LIFTED
    Chess::Square rookFromSquare;  // rook origin
    Chess::Square rookToSquare;    // rook destination
    Chess::Square kingToSquare;    // king destination
    // Set during castling placement phase
    bool kingPlaced, rookPlaced;

    // Promotion related logic
    Chess::Square promotionFrom;
    Chess::Square promotionTo;
    bool promotionIsCapture;
    Chess::PieceType promotionChoice;

    // internal helper methods
    /// Sets ERROR_RECOVERY state, clears move-phase, and immediately renders
    /// the mismatch highlights. Use instead of setting currentState directly.
    void enterErrorRecovery();
    /// Re-evaluates the physical board against the last valid position and
    /// updates LED highlights. Exits ERROR_RECOVERY if the board matches.
    void handleErrorRecovery();
    void updateSensorOccupancy(Chess::Square sq, bool occupied);
    void handlePiecePickup(Chess::Square sq);
    void handlePiecePlacement(Chess::Square sq);
    /// Validates, applies `move` to the board, resets move-phase, and checks
    /// for game-end conditions. Sets ERROR_RECOVERY on failure.
    void executeMove(Chess::Move move);
    /// Resets all move-phase variables to IDLE without touching board state.
    void resetMovePhase();
    /// Searches the legal-move list for a move from `from` to `to`.
    /// Returns true and sets `out` on success.
    bool findLegalMove(Chess::Square from, Chess::Square to, Chess::Move& out) const;
    // Search for en passant move from 'from' that captures pawn on 'capturedPawnSq'
    // returns true and sets 'out' on success
    bool findEnPassantMove(Chess::Square from, Chess::Square capturedPawnSq, Chess::Move& out) const;
    // Given rook square that was just lifted and king already lifted, determine legal castle
    // populate rookFromSquare, rookToSquare, kingToSquare, pendingMove
    // Return true if castle is legal
    bool resolveCastle(Chess::Square rookSq);
    bool resolveCastleFromRook(Chess::Square rookSq);
    /// Returns true if any legal move captures a piece on `targetSq`.
    // bool squareIsCaptureTarget(Chess::Square targetSq) const;
    void checkGameEndConditions();
};

extern GameManager game;