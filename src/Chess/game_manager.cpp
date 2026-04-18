#include "game_manager.h"
#include "global.h"
#include "IO/display_manager.h"

GameManager game;

GameManager::GameManager() {
    currentState = SystemState::MAIN_MENU;
    sensorOccupancy = 0;
    resetMovePhase();
}

void GameManager::init() {
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

// Public methods

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
// phase transitions:
// IDLE:
//      own piece lifted -> ATTACKER_LIFTED
//      opponent piece lifted -> ERROR
//
// ATTACKER_LIFTED
//      opponent piece lifted (normal capture) -> CAPTURED_REMOVED
//      opponent piece lifted (en passant) -> CPATURED_REMOVED
//      own rook lifted (castling) -> CASTLING_BOTH_LIFTED
//      else -> ERROR
//
// CAPTURED_REMOVED / CASTLING_ONE_PLACE
//      no pickups expected -> error
//
// CASTLING_BOTH_LIFTED
//      no pickups expected -> error

void GameManager::handlePiecePickup(Chess::Square sq) {
    // CASTLING_BOTH_LIFTED: both pieces already in hand
    // No further pickups are valid; player should be placing pieces now.
    if (movePhase == MovePhase::CASTLING_BOTH_LIFTED || movePhase == MovePhase::CASTLING_ONE_PLACED) {
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    // CAPTURED_REMOVED: attacker + captured piece both lifted
    // Next event must be a placement, not another pickup
    if (movePhase == MovePhase::CAPTURED_REMOVED) {
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    // read board to identify lifted piece
    Chess::ChessColor sideToMove = currentBoard.getSideToMove();
    Chess::ChessColor pieceColor = currentBoard.colorAt(sq);
    Chess::PieceType pieceType = currentBoard.pieceAt(sq);

    bool isOwnPiece = (pieceColor == sideToMove);
    bool isOpponentPiece = (pieceType != Chess::PieceType::None) && !isOwnPiece;

    // IDLE: only own piece may initiate a move
    if (movePhase == MovePhase::IDLE) {
        if (!isOwnPiece || pieceType == Chess::PieceType::None) {
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

    // ATTACKER_LIFTED: determine capture, enpassant, castle
    if (movePhase == MovePhase::ATTACKER_LIFTED) {
        if (isOpponentPiece) {
            // try normal capture
            Chess::Move captureMove;
            if (findLegalMove(attackingSquare, sq, captureMove)) {
                capturedSquare = sq;
                pendingMove = captureMove;
                movePhase = MovePhase::CAPTURED_REMOVED;
                return;
            }

            // try en passant
            // The square lifted is the opponent pawn's square, which is NOT
            // the move destination — findEnPassantMove maps it correctly.
            Chess::Move epMove;
            if (findEnPassantMove(attackingSquare, sq, epMove)) {
                capturedSquare = sq;   // physical pawn location
                pendingMove = epMove;  // destination is epMove.getTo()
                movePhase = MovePhase::CAPTURED_REMOVED;
                return;
            }

            // neither legal capture nor legal en passant.
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }

        if (isOwnPiece) {
            // try castling: own rook lifted while king is in hand
            // The attacker must be the king; the piece just lifted must be the rook on the correct side
            if (currentBoard.pieceAt(attackingSquare) != Chess::PieceType::King) {
                // not a castling attempt - illegal move
                currentState = SystemState::ERROR_RECOVERY;
                return;
            }

            if (pieceType != Chess::PieceType::Rook) {
                currentState = SystemState::ERROR_RECOVERY;
                return;
            }

            if (!resolveCastle(sq)) {
                currentState = SystemState::ERROR_RECOVERY;
                return;
            }

            movePhase = MovePhase::CASTLING_BOTH_LIFTED;
            return;
        }

        // Square is empty on the logical board — invalid pickup.
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }
}

// Piece placement
//
// Phase transitions on placement:
//
//   IDLE
//     any placement -> ERROR
//
//   ATTACKER_LIFTED
//     placed back on origin -> IDLE (cancel)
//     placed on legal non-capture destination -> executeMove -> IDLE
//     placed on occupied square -> ERROR (should have lifted opp. piece first)
//
//   CAPTURED_REMOVED
//     placed on pendingMove.getTo() -> executeMove → IDLE
//     anything else -> ERROR
//
//   CASTLING_BOTH_LIFTED
//     placed on kingToSquare -> CASTLING_ONE_PLACED (kingPlaced=true)
//     placed on rookFromSquare (rook dest) -> CASTLING_ONE_PLACED (rookPlaced=true)
//     anything else -> ERROR
//
//   CASTLING_ONE_PLACED
//     completes the remaining piece -> executeMove -> IDLE
//     anything else -> ERROR
void GameManager::handlePiecePlacement(Chess::Square sq) {
    // IDLE: nothing in hand
    if (movePhase == MovePhase::IDLE) {
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    // ATTACKER_LIFTED
    if (movePhase == MovePhase::ATTACKER_LIFTED) {
        // cancel move
        if (sq == attackingSquare) {
            resetMovePhase();
            return;
        }

        // skipped lifting opponent piece (shouldn't be physically possible)
        if (currentBoard.pieceAt(sq) != Chess::PieceType::None) {
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }

        // normal non capture move
        Chess::Move move;
        if (!findLegalMove(attackingSquare, sq, move)) {
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }

        executeMove(move);
        return;
    }

    // CAPTURED_REMOVED
    if (movePhase == MovePhase::CAPTURED_REMOVED) {
        // Attacker must land exactly on the move destination (diagonal square
        // for en passant, captured square for normal captures)
        if (sq != pendingMove.getTo()) {
            currentState = SystemState::ERROR_RECOVERY;
            return;
        }

        // pendingMove was already validated in handlePiecePickup.
        executeMove(pendingMove);
        return;
    }

    // CASTLING_BOTH_LIFTED
    if (movePhase == MovePhase::CASTLING_BOTH_LIFTED) {
        if (sq == kingToSquare) {
            kingPlaced = true;
            rookPlaced = false;
            movePhase = MovePhase::CASTLING_ONE_PLACED;
            return;
        }
        if (sq == rookToSquare) {
            rookPlaced = true;
            kingPlaced = false;
            movePhase = MovePhase::CASTLING_ONE_PLACED;
            return;
        }
        // Placed on an unexpected square.
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    // CASTLING_ONE_PLACED
    if (movePhase == MovePhase::CASTLING_ONE_PLACED) {
        if (kingPlaced) {
            // Rook must be placed on rookToSquare.
            if (sq != rookToSquare) {
                currentState = SystemState::ERROR_RECOVERY;
                return;
            }
        } else {
            // Rook was placed first; king must come down on kingToSquare
            if (sq != kingToSquare) {
                currentState = SystemState::ERROR_RECOVERY;
                return;
            }
        }

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
    rookFromSquare = Chess::SQ_NONE;
    rookToSquare = Chess::SQ_NONE;
    kingToSquare = Chess::SQ_NONE;
    kingPlaced = false;
    rookPlaced = false;
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

bool GameManager::findEnPassantMove(Chess::Square from, Chess::Square capturedPawnSq, Chess::Move& out) const {
    Chess::MoveList moves;
    const_cast<Chess::Board&>(currentBoard).generateLegalMoves(moves);

    for (size_t i = 0; i < moves.size(); ++i) {
        const Chess::Move& mv = moves[i];
        if (mv.getFlags() != 5) continue;  // flag 5 = ep-capture
        if (mv.getFrom() != from) continue;

        // The captured pawn sits on the same rank as the attacker (from),
        // and the same file as the move destination.
        int epRank = static_cast<int>(from) / 8;
        int epFile = static_cast<int>(mv.getTo()) % 8;
        Chess::Square epPawnSq = static_cast<Chess::Square>(epRank * 8 + epFile);

        if (epPawnSq == capturedPawnSq) {
            out = mv;
            return true;
        }
    }
    return false;
}

bool GameManager::resolveCastle(Chess::Square rookSq) {
    // Determine castling side from the rook square, then look for the
    // corresponding legal castle move (king moves two squares)
    Chess::ChessColor side = currentBoard.getSideToMove();
    bool isWhite = (side == Chess::ChessColor::White);

    // Expected rook origins and the resulting king/rook destinations.
    // White kingside:  king e1→g1, rook h1→f1
    // White queenside: king e1→c1, rook a1→d1
    // Black kingside:  king e8→g8, rook h8→f8
    // Black queenside: king e8→c8, rook a8→d8
    struct CastleInfo {
        Chess::Square rookFrom;
        Chess::Square rookTo;
        Chess::Square kingTo;
        uint16_t moveFlag;  // 2 = kingside, 3 = queenside
    };

    CastleInfo options[2];
    if (isWhite) {
        options[0] = {Chess::SQ_H1, Chess::SQ_F1, Chess::SQ_G1, 2};
        options[1] = {Chess::SQ_A1, Chess::SQ_D1, Chess::SQ_C1, 3};
    } else {
        options[0] = {Chess::SQ_H8, Chess::SQ_F8, Chess::SQ_G8, 2};
        options[1] = {Chess::SQ_A8, Chess::SQ_D8, Chess::SQ_C8, 3};
    }

    for (const CastleInfo& opt : options) {
        if (rookSq != opt.rookFrom) continue;

        // Verify this castle is legal
        Chess::Move castleMove;
        if (!findLegalMove(attackingSquare, opt.kingTo, castleMove)) continue;
        if (castleMove.getFlags() != opt.moveFlag) continue;

        rookFromSquare = opt.rookFrom;
        rookToSquare = opt.rookTo;
        kingToSquare = opt.kingTo;
        pendingMove = castleMove;
        kingPlaced = false;
        rookPlaced = false;
        return true;
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