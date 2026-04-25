#include "game_manager.h"

#include "IO/display_manager.h"
#include "IO/led.h"
#include "global.h"

GameManager game;

GameManager::GameManager() {
    currentState = SystemState::MAIN_MENU;
    sensorOccupancy = 0;
    lastDebouncedBoard = ~0ULL;
    debounceStartTime = 0;
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
    sensorOccupancy = readBoardBitmap();

    // Set initial user settings
    players[0] = PlayerSettings{true, true};
    players[1] = PlayerSettings{true, true};

    currentState = SystemState::MAIN_MENU;
    resetMovePhase();
}

// Public methods
void GameManager::reset() {
    currentBoard.reset();
    currentState = SystemState::MAIN_MENU;
    resetMovePhase();
}

void GameManager::updateInitialization(uint64_t sensorState) {
    // Update sensor occupancy
    sensorOccupancy = sensorState;

    // Sync LEDs - unlight squares with pieces
    lightAllStartingSquares(sensorState);

    // Count pieces detected
    int pieceCount = Chess::BitUtils::countBits(sensorOccupancy);

    Serial.printf("Pieces placed: %d/32\n", pieceCount);

    // Check if all 32 pieces are in starting position
    if (pieceCount == 32) {
        // Verify pieces are in correct squares
        Chess::Bitboard expectedOccupancy = currentBoard.getOccupancy();
        if (sensorOccupancy == expectedOccupancy) {
            currentState = SystemState::PLAYING;
            Serial.println("All pieces detected! Game starting...");
            clearAllLEDs();
            flushLEDBuffer();
        } else {
            Serial.println("Warning: 32 pieces detected but positions don't match!");
        }
    }
}

void GameManager::updateBoard(uint64_t newBoard) {
    // debounce
    if (newBoard == lastDebouncedBoard) {
        Serial.println("Failed debounce");
        debounceStartTime = 0;
        return;
    }

    // change detected, start timer
    if (debounceStartTime == 0) {
        debounceStartTime = millis();
        return;
    }

    // needs to be in different state for long enough
    if (millis() - debounceStartTime < DEBOUNCE_MS) {
        return;
    }

    Serial.println("[updateBoard] -> board ACCEPTED, continuing");

    lastDebouncedBoard = static_cast<Chess::Bitboard>(newBoard);
    debounceStartTime = 0;

    // check if piece picked up or put down
    Chess::Bitboard diffBoard = sensorOccupancy ^ static_cast<Chess::Bitboard>(newBoard);
    if (diffBoard == 0) {
        Serial.println("[updateBoard] -> early return: diffBoard == 0");
        return;
    }

    int bitsDiff = Chess::BitUtils::countBits(diffBoard);
    if (bitsDiff != 1) {
        Serial.printf("[updateBoard] -> ERROR: diffBoard has %d bits\n", Chess::BitUtils::countBits(diffBoard));
        // Accept the new sensor reading so subsequent single-square changes are
        // diffed against the correct baseline, then refresh error highlights.
        sensorOccupancy = static_cast<Chess::Bitboard>(newBoard);
        enterErrorRecovery();
        return;
    }

    Chess::Square changedSq = Chess::BitUtils::getLSB(diffBoard);
    bool wasOccupied = Chess::BitUtils::readBit(sensorOccupancy, changedSq);
    bool nowOccupied = Chess::BitUtils::readBit(static_cast<Chess::Bitboard>(newBoard), changedSq);

    updateSensorOccupancy(changedSq, nowOccupied);

    // In error recovery, every sensor change re-evaluates the board against the
    // last valid position and refreshes the LED highlights accordingly.
    if (currentState == SystemState::ERROR_RECOVERY) {
        Serial.println("[updateBoard] -> ERROR_RECOVERY: re-evaluating board");
        handleErrorRecovery();
        return;
    }

    if (wasOccupied && !nowOccupied) {
        Serial.println("[updateBoard] -> calling handlePiecePickup");
        handlePiecePickup(changedSq);
    } else {
        Serial.println("[updateBoard] -> calling handlePiecePlacement");
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

void GameManager::enterErrorRecovery() {
    currentState = SystemState::ERROR_RECOVERY;

    movePhase = MovePhase::IDLE;
    attackingSquare = Chess::SQ_NONE;
    capturedSquare = Chess::SQ_NONE;
    pendingMove = Chess::Move();
    rookFromSquare = Chess::SQ_NONE;
    rookToSquare = Chess::SQ_NONE;
    kingToSquare = Chess::SQ_NONE;
    kingPlaced = false;
    rookPlaced = false;

    handleErrorRecovery();
}

void GameManager::handleErrorRecovery() {
    Chess::Bitboard expected = currentBoard.getOccupancy();

    // 3. Return to normal play state after
    if (sensorOccupancy == expected) {
        Serial.println("[ERROR_RECOVERY] Board matched expected state. Resuming PLAYING.");
        currentState = SystemState::PLAYING;
        resetMovePhase();
        return;
    }

    clearAllLEDs();

    Chess::Bitboard missing = expected & ~sensorOccupancy;
    Chess::Bitboard tempMissing = missing;
    while (tempMissing) {
        Chess::Square sq = Chess::BitUtils::popLSB(tempMissing);
        Chess::ChessColor color = currentBoard.colorAt(sq);
        uint32_t ledColor = (color == Chess::ChessColor::White) ? WHITE_PIECES : BLACK_PIECES;
        highlightSquare(sq / 8, sq % 8, ledColor);
    }

    // 2. Highlight squares that we detect an unexpected piece with ILLEGAL_PIECE_COLOR
    Chess::Bitboard unexpected = sensorOccupancy & ~expected;
    Chess::Bitboard tempUnexpected = unexpected;
    while (tempUnexpected) {
        Chess::Square sq = Chess::BitUtils::popLSB(tempUnexpected);
        highlightSquare(sq / 8, sq % 8, ILLEGAL_PIECE_COLOR);
    }

    flushLEDBuffer(true);
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
    if (movePhase == MovePhase::CASTLING_BOTH_LIFTED || movePhase == MovePhase::CAPTURED_REMOVED) {
        enterErrorRecovery();
        return;
    }

    // EP_ATTACKER_PLACED: attacker is already on the destination; waiting for
    // the physically-captured pawn to be removed from the board.
    if (movePhase == MovePhase::EP_ATTACKER_PLACED) {
        if (sq == capturedSquare) {
            // Captured pawn removed — the move is now fully committed.
            executeMove(pendingMove);
        } else {
            enterErrorRecovery();
        }
        return;
    }

    // CASTLING_ROOK_PLACED: rook is already on its destination square.
    // The only valid pickup is the king from its origin.
    if (movePhase == MovePhase::CASTLING_ROOK_PLACED) {
        if (sq == attackingSquare) {
            // King lifted — rook is already on its destination, king now in hand.
            Serial.printf("[CASTLING_ROOK_PLACED] King lifted at sq=%d\n", sq);
            movePhase = MovePhase::CASTLING_KING_LIFTED;
            kingPlaced = false;

            // Highlight only king destination (rook is already placed)
            int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
            if (players[playerIndex].showLegalMoves) {
                clearAllLEDs();
                highlightSquare(kingToSquare / 8, kingToSquare % 8, CASTLING_COLOR);
                highlightSquare(attackingSquare / 8, attackingSquare % 8, CASTLING_COLOR);
                flushLEDBuffer();
            }
        } else if (sq == rookToSquare) {
            // Player picked the rook back up from its destination — cancel back to rook lifted.
            Serial.printf("[CASTLING_ROOK_PLACED] Rook picked back up, reverting to CASTLING_ROOK_LIFTED\n");
            movePhase = MovePhase::CASTLING_ROOK_LIFTED;
            rookPlaced = false;

            int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
            if (players[playerIndex].showLegalMoves) {
                clearAllLEDs();
                highlightSquare(attackingSquare / 8, attackingSquare % 8, LIFTED_PIECE_COLOR);
                highlightSquare(kingToSquare / 8, kingToSquare % 8, CASTLING_COLOR);
                highlightSquare(rookToSquare / 8, rookToSquare % 8, CASTLING_COLOR);
                flushLEDBuffer();
            }

        } else {
            enterErrorRecovery();
        }

        return;
    }

    // CASTLING_KING_PLACED: king is already on its destination square.

    // The only valid pickup is the rook from its origin.
    if (movePhase == MovePhase::CASTLING_KING_PLACED) {
        if (sq == rookFromSquare) {
            // Rook lifted — rook in hand, king already on its destination.
            // Use CASTLING_BOTH_LIFTED with kingPlaced=true so the placement
            // handler knows only the rook needs to land.
            Serial.printf("[CASTLING_KING_PLACED] Rook lifted at sq=%d\n", sq);
            movePhase = MovePhase::CASTLING_BOTH_LIFTED;

            kingPlaced = true;
            rookPlaced = false;

            int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
            if (players[playerIndex].showLegalMoves) {
                clearAllLEDs();
                highlightSquare(rookFromSquare / 8, rookFromSquare % 8, CASTLING_COLOR);
                highlightSquare(rookToSquare / 8, rookToSquare % 8, CASTLING_COLOR);
                flushLEDBuffer();
            }
        } else if (sq == kingToSquare) {
            // Player picked the king back up from its destination — back to king-in-hand state.
            Serial.printf("[CASTLING_KING_PLACED] King picked back up, reverting to CASTLING_KING_LIFTED\n");
            movePhase = MovePhase::CASTLING_KING_LIFTED;

            kingPlaced = false;
            rookPlaced = false;

            int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
            if (players[playerIndex].showLegalMoves) {
                clearAllLEDs();
                highlightSquare(rookFromSquare / 8, rookFromSquare % 8, LIFTED_PIECE_COLOR);
                highlightSquare(kingToSquare / 8, kingToSquare % 8, CASTLING_COLOR);
                highlightSquare(rookToSquare / 8, rookToSquare % 8, CASTLING_COLOR);
                flushLEDBuffer();
            }
        } else {
            enterErrorRecovery();
        }
        return;
    }

    // CASTLING_KING_LIFTED: king in hand
    // Only valid pickup from here is the rook.
    if (movePhase == MovePhase::CASTLING_KING_LIFTED) {
        if (sq == rookFromSquare) {
            Serial.printf("[CASTLING_KING_LIFTED] Rook lifted at sq=%d, both in hand\n", sq);
            movePhase = MovePhase::CASTLING_BOTH_LIFTED;

            kingPlaced = false;
            rookPlaced = false;

            int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
            if (players[playerIndex].showLegalMoves) {
                clearAllLEDs();
                highlightSquare(kingToSquare / 8, kingToSquare % 8, CASTLING_COLOR);
                highlightSquare(rookToSquare / 8, rookToSquare % 8, CASTLING_COLOR);
                flushLEDBuffer();
            }
        } else {
            enterErrorRecovery();
        }
        return;
    }

    // read board to identify lifted piece
    Chess::ChessColor sideToMove = currentBoard.getSideToMove();
    Chess::ChessColor pieceColor = currentBoard.colorAt(sq);
    Chess::PieceType pieceType = currentBoard.pieceAt(sq);

    // uiManager->showPickedUpPiece(pieceType, sideToMove);

    bool isOwnPiece = (pieceColor == sideToMove);
    bool isOpponentPiece = (pieceType != Chess::PieceType::None) && !isOwnPiece;

    // CASTLING_ROOK_LIFTED: rook is already in hand, now waiting for the king.
    if (movePhase == MovePhase::CASTLING_ROOK_LIFTED) {
        if (!isOwnPiece || pieceType != Chess::PieceType::King) {
            Serial.printf("[CASTLING_ROOK_LIFTED] ERROR: expected own King, got pieceType=%d isOwn=%d sq=%d\n",
                          (int)pieceType, isOwnPiece, sq);
            enterErrorRecovery();
            return;
        }
        // King lifted — both pieces now in hand.
        // attackingSquare (king origin) was stored by resolveCastleFromRook.
        Serial.printf("[CASTLING_ROOK_LIFTED] King lifted at sq=%d, both pieces in hand\n", sq);
        movePhase = MovePhase::CASTLING_BOTH_LIFTED;
        return;
    }

    // IDLE: only own piece may initiate a move
    if (movePhase == MovePhase::IDLE) {
        if (!isOwnPiece || pieceType == Chess::PieceType::None) {
            // Touching the opponent's piece before your own is illegal.
            enterErrorRecovery();
            return;
        }

        // Record attacker and advance phase.
        attackingSquare = sq;
        movePhase = MovePhase::ATTACKER_LIFTED;

        int playerIndex = (sideToMove == Chess::ChessColor::White) ? 0 : 1;
        if (players[playerIndex].showLegalMoves) {
            highlightLegalMoves(attackingSquare, currentBoard);
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
            enterErrorRecovery();
            return;
        }

        if (isOwnPiece) {
            // try castling: own rook lifted while king is in hand
            // The attacker must be the king; the piece just lifted must be the rook on the correct side
            if (currentBoard.pieceAt(attackingSquare) != Chess::PieceType::King) {
                // not a castling attempt - illegal move
                enterErrorRecovery();
                return;
            }

            if (pieceType != Chess::PieceType::Rook) {
                enterErrorRecovery();
                return;
            }

            if (!resolveCastle(sq)) {
                Serial.printf(
                    "[ATTACKER_LIFTED] ERROR: own piece lifted at sq=%d but castle resolve failed (attacker=%d, "
                    "pieceType=%d)\n",
                    sq, attackingSquare, (int)pieceType);
                enterErrorRecovery();
                return;
            }
            Serial.printf("[ATTACKER_LIFTED] Castle resolved: kingTo=%d rookTo=%d\n", kingToSquare, rookToSquare);

            movePhase = MovePhase::CASTLING_BOTH_LIFTED;

            int playerIndex = (sideToMove == Chess::ChessColor::White) ? 0 : 1;
            if (players[playerIndex].showLegalMoves) {
                clearAllLEDs();
                highlightSquare(kingToSquare / 8, kingToSquare % 8, CASTLING_COLOR);
                highlightSquare(rookToSquare / 8, rookToSquare % 8, CASTLING_COLOR);
                flushLEDBuffer();
            }

            return;
        }

        // Square is empty on the logical board — invalid pickup.
        enterErrorRecovery();
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
        enterErrorRecovery();
        return;
    }

    // CASTLING_ROOK_LIFTED: rook is in hand, king hasn't been lifted yet.
    // Either put on destination or cancel
    if (movePhase == MovePhase::CASTLING_ROOK_LIFTED) {
        if (sq == rookFromSquare) {
            Serial.printf("[CASTLING_ROOK_LIFTED] Rook returned to origin, cancelling\n");
            resetMovePhase();
            return;
        }
        if (sq == rookToSquare) {
            Serial.printf("[CASTLING_ROOK_LIFTED] Rook placed on sq=%d (method 2: rook first), waiting for king\n", sq);
            movePhase = MovePhase::CASTLING_ROOK_PLACED;
            rookPlaced = true;
            // Clear rook destination highlight; king destination remains to guide player
            int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
            if (players[playerIndex].showLegalMoves) {
                clearAllLEDs();
                highlightSquare(kingToSquare / 8, kingToSquare % 8, CASTLING_COLOR);
                flushLEDBuffer();
            }
            return;
        }
        Serial.printf("[CASTLING_ROOK_LIFTED] ERROR: placement on sq=%d (rookFrom=%d, rookTo=%d, kingTo=%d)\n", sq,
                      rookFromSquare, rookToSquare, kingToSquare);
        enterErrorRecovery();
        return;
    }

    // CASTLING_ROOK_PLACED: rook on dest square
    if (movePhase == MovePhase::CASTLING_ROOK_PLACED) {
        if (sq == kingToSquare) {
            Serial.printf("[CASTLING_ROOK_PLACED] King placed on sq=%d, castle complete\n", sq);
            executeMove(pendingMove);
            return;
        }
        Serial.printf("[CASTLING_ROOK_PLACED] ERROR: unexpected placement on sq=%d (expected kingTo=%d)\n", sq,
                      kingToSquare);
        enterErrorRecovery();
        return;
    }

    // CASTLING_KING_LIFTED: king is in hand, reached via:
    //   - CASTLING_ROOK_PLACED (rook already on dest, rookPlaced=true)
    //   - CASTLING_KING_PLACED king-picked-back-up (rookPlaced=false)
    // rookPlaced=true means the rook is already on rookToSquare.
    if (movePhase == MovePhase::CASTLING_KING_LIFTED) {
        if (sq == attackingSquare) {
            if (rookPlaced) {
                // Rook is already on its destination; king returned to origin.
                // Revert to CASTLING_ROOK_PLACED so the player can pick the rook back up to fully cancel.
                Serial.printf("[CASTLING_KING_PLACED] Rook lifted at sq=%d, both in hand\n", sq);
                movePhase = MovePhase::CASTLING_BOTH_LIFTED;

                kingPlaced = false;
                rookPlaced = false;

                int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
                if (players[playerIndex].showLegalMoves) {
                    clearAllLEDs();
                    highlightSquare(kingToSquare / 8, kingToSquare % 8, CASTLING_COLOR);
                    flushLEDBuffer();
                }
            } else {
                Serial.printf("[CASTLING_KING_PLACED] King picked back up, reverting to CASTLING_KING_LIFTED\n");
                movePhase = MovePhase::CASTLING_KING_LIFTED;

                kingPlaced = false;
                rookPlaced = false;

                int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
                if (players[playerIndex].showLegalMoves) {
                    clearAllLEDs();
                    highlightSquare(rookFromSquare / 8, rookFromSquare % 8, LIFTED_PIECE_COLOR);
                    highlightSquare(kingToSquare / 8, kingToSquare % 8, CASTLING_COLOR);
                    highlightSquare(rookToSquare / 8, rookToSquare % 8, CASTLING_COLOR);
                    flushLEDBuffer();
                }
            }
            return;
        }
        if (sq == kingToSquare) {
            if (rookPlaced) {
                // Rook is already on rookToSquare; king now on kingToSquare — castle complete!
                Serial.printf(
                    "[CASTLING_KING_LIFTED] King placed on sq=%d with rook already placed, executing castle\n", sq);
                executeMove(pendingMove);
            } else {
                // Rook hasn't moved yet; wait for it.
                Serial.printf("[CASTLING_KING_LIFTED] King placed on sq=%d, waiting for rook\n", sq);
                movePhase = MovePhase::CASTLING_KING_PLACED;

                kingPlaced = true;
                rookPlaced = false;

                int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
                if (players[playerIndex].showLegalMoves) {
                    clearAllLEDs();
                    highlightSquare(rookFromSquare / 8, rookFromSquare % 8, CASTLING_COLOR);
                    highlightSquare(rookToSquare / 8, rookToSquare % 8, CASTLING_COLOR);
                    flushLEDBuffer();
                }
            }
            return;
        }
        Serial.printf("[CASTLING_KING_LIFTED] ERROR: placement on sq=%d\n", sq);
        enterErrorRecovery();
        return;
    }

    // CASTLING_KING_PLACED: king is on its destination, waiting for rook to be
    // picked up (handled in handlePiecePickup). No placement is valid here.
    if (movePhase == MovePhase::CASTLING_KING_PLACED) {
        if (sq == rookToSquare) {
            Serial.printf("[CASTLING_KING_PLACED] Rook placed on sq=%d, castle complete\n", sq);
            executeMove(pendingMove);
            return;
        }
        Serial.printf("[CASTLING_KING_PLACED] ERROR: unexpected placement on sq=%d (expected rookTo=%d)\n", sq,
                      rookToSquare);
        enterErrorRecovery();
        return;
    }

    // ATTACKER_LIFTED
    if (movePhase == MovePhase::ATTACKER_LIFTED) {
        // cancel move
        if (sq == attackingSquare) {
            resetMovePhase();
            return;
        }

        // check for castling
        if (currentBoard.pieceAt(attackingSquare) == Chess::PieceType::King) {
            if (sq == Chess::SQ_G1 || sq == Chess::SQ_G8 || sq == Chess::SQ_C1 || sq == Chess::SQ_C8) {
                Chess::Move castleMove;
                if (!findLegalMove(attackingSquare, sq, castleMove)) {
                    enterErrorRecovery();
                    return;
                }
                uint16_t flag = castleMove.getFlags();
                if (flag < 2 || flag > 3) {
                    enterErrorRecovery();
                    return;
                }

                // Resolve rook squares from the castle flag
                Chess::ChessColor side = currentBoard.getSideToMove();
                bool isWhite = (side == Chess::ChessColor::White);
                if (flag == 2) {  // kingside
                    rookFromSquare = isWhite ? Chess::SQ_H1 : Chess::SQ_H8;
                    rookToSquare = isWhite ? Chess::SQ_F1 : Chess::SQ_F8;
                    kingToSquare = isWhite ? Chess::SQ_G1 : Chess::SQ_G8;
                } else {  // queenside
                    rookFromSquare = isWhite ? Chess::SQ_A1 : Chess::SQ_A8;
                    rookToSquare = isWhite ? Chess::SQ_D1 : Chess::SQ_D8;
                    kingToSquare = isWhite ? Chess::SQ_C1 : Chess::SQ_C8;
                }

                Serial.printf("[ATTACKER_LIFTED] King placed on castling square sq=%d, flag=%d\n", sq, flag);
                pendingMove = castleMove;
                kingPlaced = true;
                rookPlaced = false;
                movePhase = MovePhase::CASTLING_KING_PLACED;
                // Highlight rook destination (only step remaining)
                int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
                if (players[playerIndex].showLegalMoves) {
                    clearAllLEDs();
                    highlightSquare(rookFromSquare / 8, rookFromSquare % 8, CASTLING_COLOR);
                    highlightSquare(rookToSquare / 8, rookToSquare % 8, CASTLING_COLOR);
                    flushLEDBuffer();
                }
                return;
            }
        }

        // Check if this is an en passant destination placed before the captured
        // pawn is removed (attacker-first en passant sequence).
        Chess::Move epMove;
        if (findLegalMove(attackingSquare, sq, epMove) && epMove.getFlags() == 5) {
            // Attacker placed on EP destination; now wait for captured pawn removal.
            pendingMove = epMove;
            // Compute the captured pawn's square: same rank as attacker origin,
            // same file as destination.
            int epRank = static_cast<int>(attackingSquare) / 8;
            int epFile = static_cast<int>(sq) % 8;
            capturedSquare = static_cast<Chess::Square>(epRank * 8 + epFile);
            movePhase = MovePhase::EP_ATTACKER_PLACED;
            clearAllLEDs();
            highlightSquare(capturedSquare / 8, capturedSquare % 8, ILLEGAL_PIECE_COLOR);
            flushLEDBuffer();
            return;
        }

        // normal non capture move
        Chess::Move move;
        if (!findLegalMove(attackingSquare, sq, move)) {
            enterErrorRecovery();
            return;
        }

        // promotion path
        if (move.isPromotion()) {
            promotionFrom = move.getFrom();
            promotionTo = move.getTo();
            promotionIsCapture = move.isCapture();  // should be false
            promotionChoice = Chess::PieceType::None;
            currentState = SystemState::PROMOTION_SELECTION;
            resetMovePhase();
            return;
        }

        executeMove(move);
        return;
    }

    // EP_ATTACKER_PLACED: attacker is already on the destination square;
    // wait for the captured pawn to be physically removed
    if (movePhase == MovePhase::EP_ATTACKER_PLACED) {
        // No placement is valid while waiting for the captured pawn removal
        enterErrorRecovery();
        return;
    }

    // CAPTURED_REMOVED
    if (movePhase == MovePhase::CAPTURED_REMOVED) {
        // Attacker must land exactly on the move destination (diagonal square
        // for en passant, captured square for normal captures)
        if (sq != pendingMove.getTo()) {
            enterErrorRecovery();
            return;
        }

        // promotion path
        if (pendingMove.isPromotion()) {
            promotionFrom = pendingMove.getFrom();
            promotionTo = pendingMove.getTo();
            promotionIsCapture = true;
            promotionChoice = Chess::PieceType::None;
            currentState = SystemState::PROMOTION_SELECTION;
            resetMovePhase();
            // Optionally turn off all LEDs or show a “promotion pending” indicator
            return;
        }

        // pendingMove was already validated in handlePiecePickup.
        executeMove(pendingMove);
        return;
    }

    // CASTLING_BOTH_LIFTED
    if (movePhase == MovePhase::CASTLING_BOTH_LIFTED) {
        if (!kingPlaced && sq == kingToSquare) {
            // King placed first; rook still in hand.
            Serial.printf("[CASTLING_BOTH_LIFTED] King placed on sq=%d, waiting for rook\n", sq);
            kingPlaced = true;
            rookPlaced = false;
            movePhase = MovePhase::CASTLING_KING_PLACED;

            // Clear king destination highlight; rook destination stays
            int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
            if (players[playerIndex].showLegalMoves) {
                clearAllLEDs();
                highlightSquare(rookToSquare / 8, rookToSquare % 8, CASTLING_COLOR);
                flushLEDBuffer();
            }
            return;
        }
        if (!rookPlaced && sq == rookToSquare) {
            // Rook placed first (or completing when king already placed).
            if (kingPlaced) {
                // King was already placed and we just placed the rook — castle complete.
                Serial.printf("[CASTLING_BOTH_LIFTED] Rook placed on sq=%d, castle complete\n", sq);
                executeMove(pendingMove);
            } else {
                Serial.printf("[CASTLING_BOTH_LIFTED] Rook placed on sq=%d, waiting for king\n", sq);
                rookPlaced = true;
                kingPlaced = false;
                movePhase = MovePhase::CASTLING_ROOK_PLACED;

                int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
                if (players[playerIndex].showLegalMoves) {
                    clearAllLEDs();
                    highlightSquare(kingToSquare / 8, kingToSquare % 8, CASTLING_COLOR);
                    flushLEDBuffer();
                }
            }
            return;
        }
        // Placed on an unexpected square.
        enterErrorRecovery();
        return;
    }
}

// move execution
void GameManager::executeMove(Chess::Move move) {
    bool success = currentBoard.makeMove(move);
    if (!success) {
        enterErrorRecovery();
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
    clearAllLEDs();
    flushLEDBuffer(true);
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
    Serial.print("called resolveCastle()");
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
    Serial.print("resolveCastle() returns false");

    return false;
}

bool GameManager::resolveCastleFromRook(Chess::Square rookSq) {
    // Like resolveCastle, but the rook is lifted first.
    Chess::ChessColor side = currentBoard.getSideToMove();
    bool isWhite = (side == Chess::ChessColor::White);

    struct CastleInfo {
        Chess::Square kingFrom;
        Chess::Square rookFrom;
        Chess::Square rookTo;
        Chess::Square kingTo;
        uint16_t moveFlag;
    };

    CastleInfo options[2];
    if (isWhite) {
        options[0] = {Chess::SQ_E1, Chess::SQ_H1, Chess::SQ_F1, Chess::SQ_G1, 2};
        options[1] = {Chess::SQ_E1, Chess::SQ_A1, Chess::SQ_D1, Chess::SQ_C1, 3};
    } else {
        options[0] = {Chess::SQ_E8, Chess::SQ_H8, Chess::SQ_F8, Chess::SQ_G8, 2};
        options[1] = {Chess::SQ_E8, Chess::SQ_A8, Chess::SQ_D8, Chess::SQ_C8, 3};
    }

    for (const CastleInfo& opt : options) {
        if (rookSq != opt.rookFrom) continue;

        // Verify piece at king origin really is the king.
        if (currentBoard.pieceAt(opt.kingFrom) != Chess::PieceType::King) continue;
        if (currentBoard.colorAt(opt.kingFrom) != side) continue;

        // Verify the castle move is legal (king travels two squares).
        Chess::Move castleMove;
        if (!findLegalMove(opt.kingFrom, opt.kingTo, castleMove)) continue;
        if (castleMove.getFlags() != opt.moveFlag) continue;

        // Store all the castling state; attackingSquare = king origin.
        attackingSquare = opt.kingFrom;
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
    if (currentBoard.isThreefoldRepetition()) {
        currentState = SystemState::GAME_OVER;
        return;
    }
    if (currentBoard.isInsufficientMaterial()) {
        currentState = SystemState::GAME_OVER;
        return;
    }

    // Show best move hint for the side now to move, if enabled.
    int playerIndex = (currentBoard.getSideToMove() == Chess::ChessColor::White) ? 0 : 1;
    if (players[playerIndex].showBestMove) {
        // TODO: show best move
    }
}

void GameManager::finalizePromotion() {
    if (promotionChoice == Chess::PieceType::None) return;

    if (!Chess::BitUtils::readBit(sensorOccupancy, promotionTo)) {
        // Pawn was removed unexpectedly
        enterErrorRecovery();
        promotionChoice = Chess::PieceType::None;
        return;
    }

    // Convert chosen piece type to move flags according to promotion encoding.
    // The flag table: quiet promotions = 8..11; capture promotions = 12..15.
    // For each, lower two bits encode the piece: Knight=0, Bishop=1, Rook=2, Queen=3.
    int pieceIndex = static_cast<int>(promotionChoice) - static_cast<int>(Chess::PieceType::Knight);
    // pieceChoice is Knight/Bishop/Rook/Queen: map them to 0-3
    uint16_t baseFlag = promotionIsCapture ? 12 : 8;
    uint16_t flags = baseFlag + pieceIndex;

    Chess::Move finalMove(promotionFrom, promotionTo, flags);

    bool ok = currentBoard.makeMove(finalMove);
    if (!ok) {
        enterErrorRecovery();
        resetMovePhase();
        return;
    }

    currentState = SystemState::PLAYING;
    resetMovePhase();
    // Notify the UI task that state changed (so it exits the promotion menu)
    // The game loop will handle that notification in its next iteration.
    checkGameEndConditions();
}

PlayerSettings& GameManager::getSettings(uint8_t player) { return players[player]; }

void GameManager::setSettings(uint8_t playerNum, bool showBestMove, bool showLegalMoves) {
    // out of bounds checking
    if (playerNum >= 2 || playerNum < 0) return;

    players[playerNum].showBestMove = showBestMove;
    players[playerNum].showLegalMoves = showLegalMoves;
}

Chess::Board& GameManager::getBoard() { return currentBoard; }
SystemState GameManager::getState() { return currentState; }