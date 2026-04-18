#include "game_manager.h"

GameManager game;

GameManager::GameManager() {
    currentState = SystemState::MAIN_MENU;
    resetMovePhase();
    sensorOccupancy = 0;
}

void GameManager::init() {
    // Initilize attack tables
    static bool attacksInitialized = false;
    if (!attacksInitialized) {
        Chess::Attacks::init();
        attacksInitialized = true;
    }

    // Set chessboard to starting position
    currentBoard.reset();

    // set sensor occupancy to starting position
    sensorOccupancy = currentBoard.getOccupancy();

    // Set initial user settings
    players[0] = PlayerSettings{false, false};
    players[1] = PlayerSettings{false, false};

    currentState = SystemState::PLAYING;
    resetMovePhase();
}

void GameManager::updateBoard(uint64_t newBoard) {
    // check if piece picked up or put down
    Chess::Bitboard diffBoard = sensorOccupancy ^ static_cast<Chess::Bitboard>(newBoard);
    if (diffBoard == 0) return;

    if (Chess::BitUtils::countBits(diffBoard) != 1) {
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    Chess::Square changedSq = Chess::BitUtils::getLSB(diffBoard);
    bool wasOccupied = Chess::BitUtils::readBit(sensorOccupancy, changedSq);
    bool nowOccupied = Chess::BitUtils::readBit(static_cast<Chess::Bitboard>(newBoard), changedSq);

    updateSensorOccupancy(changedSq, nowOccupied);

    if (wasOccupied && !nowOccupied) {
        // Piece picked up
        handlePiecePickup(changedSq);
    } else {
        handlePiecePlacement(changedSq);
    }
}

// sensor helper
void GameManager::updateSensorOccupancy(Chess::Square sq, bool occupied) {
    if (occupied) {
        Chess::BitUtils::setBit(sensorOccupancy, sq);
    } else {
        Chess::BitUtils::clearBit(sensorOccupancy, sq);
    }
}

// piece pickup
// sequence:
// Normal move: lift own piece
// Capture:     life own piece
//              lift opponent piece

void GameManager::handlePiecePickup(Chess::Square sq) {
    Chess::ChessColor sideToMove = currentBoard.getSideToMove();
    Chess::ChessColor pieceColor = currentBoard.colorAt(sq);
    Chess::PieceType pieceType = currentBoard.pieceAt(sq);

    if (pieceType == Chess::PieceType::None) {
        // Sensor fired on an empty square
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    bool isOwnPiece = (pieceColor == sideToMove);
    bool isOpponentPiece = !isOwnPiece;

    // IDLE: only own piece may initiate a move
    if (movePhase == MovePhase::IDLE) {
        if (isOpponentPiece) {
            // Touching the opponent's piece before your own is illegal.
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }

        // Record attacker and advance phase.
        attackingSquare = sq;
        movePhase = MovePhase::ATTACKER_LIFTED;

        int playerIndex = (sideToMove == Chess::ChessColor::White) ? 0 : 1;
        if (players[playerIndex].showLegalMoves) {
            // TODO: highlight legal destination squares for attackingSquare
        }
        return;
    }

    // ATTACKER_LIFTED: only valid second pickup is capture target
    if (movePhase == MovePhase::ATTACKER_LIFTED) {
        if (isOwnPiece) {
            // Player lifted a second friendly piece
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }

        // The opponent piece being lifted must be a legal capture target for the piece already held
        Chess::Move captureMove;
        if (!findLegalMove(attackingSquare, sq, captureMove)) {
            // square is not a valid capture destination
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }

        // Store everything needed to complete the capture on placement.
        capturedSquare = sq;
        pendingMove = captureMove;
        movePhase = MovePhase::CAPTURED_REMOVED;
        return;
    }

    // CAPTURED_REMOVED: no further pickups are expected
    // (The attacker is in hand and the captured piece has been removed.)
    currentState = SystemState::ERROR_RECOVERY;
}

// Piece placement
// Expected sequence:
// Cancel:          place own piece back to origin square
// Normal move:     place own piece on legal square
// Capture:         place own piece on capturedSquare
void GameManager::handlePiecePlacement(Chess::Square sq) {
    // nothing in hand
    if (movePhase == MovePhase::IDLE) {
        // A piece appeared on the board without us tracking a pickup.
        // This can happen if a captured piece is returned to the board off-turn.
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    // cancel move: piece picked up and placed back down same place
    if (sq == attackingSquare) {
        // Player changed their mind and put the piece back.
        // If we already removed a captured piece this is invalid
        if (movePhase == MovePhase::CAPTURED_REMOVED) {
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }
        resetMovePhase();
        return;
    }
    // ATTACKER_LIFTED: resolve destination for a normal (non-capture) move
    if (movePhase == MovePhase::ATTACKER_LIFTED) {
        Chess::Move move;
        if (!findLegalMove(attackingSquare, sq, move)) {
            // Not a legal destination
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }

        if (move.isCapture()) {
            // Player placed on an occupied square without first removing the
            // opponent piece.  This shouldn't happen with physical switches.
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }

        executeMove(move);
        return;
    }

    // CAPTURED_REMOVED: attacker must land on the captured square
    if (movePhase == MovePhase::CAPTURED_REMOVED) {
        if (sq != capturedSquare) {
            // The attacker was placed somewhere other than the capture square
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }

        // pendingMove was already validated in handlePiecePickup.
        executeMove(pendingMove);
        return;
    }
}

// move execution
void GameManager::executeMove(Chess::Move move) {
    bool success = currentBoard.makeMove(move);
    if (!success) {
        currentState = SystemState::ERROR_RECOVERY;
        resetMovePhase();
        return;
    }

    resetMovePhase();
    checkGameEndConditions();
}

//
// helpers
//

void GameManager::resetMovePhase() {
    movePhase = MovePhase::IDLE;
    attackingSquare = Chess::SQ_NONE;
    capturedSquare = Chess::SQ_NONE;
    pendingMove = Chess::Move();
}

bool GameManager::findLegalMove(Chess::Square from, Chess::Square to, Chess::Move& out) const {
    Chess::MoveList moves;
    const_cast<Chess::Board&>(currentBoard).generateLegalMoves(moves);

    for (size_t i = 0; i < moves.size(); ++i) {
        if (moves[i].getFrom() == from && moves[i].getTo() == to) {
            out = moves[i];
            return true;
        }
    }
    return false;
}

bool GameManager::squareIsCaptureTarget(Chess::Square targetSq) const {
    Chess::MoveList moves;
    const_cast<Chess::Board&>(currentBoard).generateLegalMoves(moves);

    for (size_t i = 0; i < moves.size(); ++i) {
        if (moves[i].isCapture() && moves[i].getTo() == targetSq) {
            return true;
        }
    }
    return false;
}

void GameManager::checkGameEndConditions() {
    if (currentBoard.isCheckmate()) {
        currentState = SystemState::GAME_OVER;
        return;
    }
    if (currentBoard.isStalemate()) {
        currentState = SystemState::GAME_OVER;
        return;
    }

    // Show best move hint for the side now to move, if enabled.
    int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
    if (players[playerIndex].showBestMove) {
        // TODO: show best move
    }
}

PlayerSettings& GameManager::getSettings(uint8_t player) { return players[player]; }

void GameManager::setSettings(uint8_t playerNum, bool showBestMove, bool showLegalMoves) {
    // out of bounds checking
    if (playerNum >= 2 || playerNum < 0) return;

    players[playerNum].showBestMove = showBestMove;
    players[playerNum].showLegalMoves = showLegalMoves;
}

Chess::Board& GameManager::getBoard() { return currentBoard; }