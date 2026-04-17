#include "game_manager.h"

GameManager game;

GameManager::GameManager() {
    currentState = SystemState::MAIN_MENU;
    currentBoard = Chess::Board();
    hasPickedUpPiece = false;
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

    pickupSquare = Chess::SQ_NONE;
    pickedUpPiece = Chess::PieceType::None;
    pickedUpColor = Chess::ChessColor::None;
}

void GameManager::updateBoard(uint64_t newBoard) {
    // check if piece picked up or put down
    Chess::Bitboard prevOccupied = sensorOccupancy;
    Chess::Bitboard diffBoard = prevOccupied ^ newBoard;
    int numChanged = Chess::BitUtils::countBits(diffBoard);

    if (numChanged == 0) return;

    if (numChanged > 1) {
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    Chess::Square changedSquare = Chess::BitUtils::getLSB(diffBoard);
    bool wasOccupied = Chess::BitUtils::readBit(prevOccupied, changedSquare);
    bool nowOccupied = Chess::BitUtils::readBit(newBoard, changedSquare);

    // update sensor tracking
    updateSensorOccupancy(changedSquare, nowOccupied);

    if (wasOccupied && !nowOccupied) {
        // Piece picked up
        handlePiecePickup(changedSquare);
    } else if (!wasOccupied && nowOccupied) {
        // Piece placed
        handlePiecePlacement(changedSquare);
    }
}

void GameManager::updateSensorOccupancy(Chess::Square square, bool occupied) {
    if (occupied) {
        Chess::BitUtils::setBit(sensorOccupancy, square);
    } else {
        Chess::BitUtils::clearBit(sensorOccupancy, square);
    }
}

void GameManager::handlePiecePickup(Chess::Square square) {
    if (hasPickedUpPiece) {
        // already has a piece picked up
        // IMPORTANT: Edit for castling and captures
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    // validate move
    Chess::PieceType piece = currentBoard.pieceAt(square);
    Chess::ChessColor color = currentBoard.colorAt(square);

    if (piece == Chess::PieceType::None) {
        // no piece here, sensor mismatch
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    if (color != currentBoard.getSideToMove()) {
        // must be right color move
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    // Store pickup
    pickupSquare = square;
    pickedUpPiece = currentBoard.pieceAt(square);
    pickedUpColor = currentBoard.colorAt(square);
    hasPickedUpPiece = true;

    if (players[currentBoard.getSideToMove() == Chess::ChessColor::White ? 0 : 1].showLegalMoves) {
        // TODO: Highlight legal moves for piece if showLegalMoves enabled
    }
}

void GameManager::handlePiecePlacement(Chess::Square square) {
    if (!hasPickedUpPiece) {
        // no piece picked up
        currentState = SystemState::ERROR_RECOVERY;
        return;
    }

    // cancel move: piece picked up and placed back down same place
    if (square == pickupSquare) {
        hasPickedUpPiece = false;
        pickupSquare = Chess::SQ_NONE;
        return;
    }

    // try to match legal move
    Chess::MoveList moves;
    currentBoard.generateLegalMoves(moves);

    Chess::Move matchingMove;
    bool foundLegalMove = false;

    // check for exact match
    for (size_t i = 0; i < moves.size(); i++) {
        Chess::Move mv = moves[i];
        if (mv.getFrom() == pickupSquare && mv.getTo() == square) {
            matchingMove = mv;
            foundLegalMove = true;
            break;
        }
    }

    if (!foundLegalMove) {
        // could be castling
        hasPickedUpPiece = false;
        return;
    }

    // make move on board
    bool success = currentBoard.makeMove(matchingMove);
    if (!success) {
        currentState = SystemState::ERROR_RECOVERY;
        hasPickedUpPiece = false;
        return;
    }

    // update game state
    checkGameEndConditions();

    // reset pickup state
    hasPickedUpPiece = false;
    pickupSquare = Chess::SQ_NONE;
}

void GameManager::checkGameEndConditions() {
    // TODO: game over stuff
    if (currentBoard.isCheckmate()) {
        // Game over - checkmate
        // Could transition to game over state
        currentState = SystemState::GAME_OVER;
    } else if (currentBoard.isStalemate()) {
        // Game over - stalemate
        currentState = SystemState::GAME_OVER;
    }

    // show best move if enabled
    int playerIndex = currentBoard.getSideToMove() == Chess::ChessColor::White ? 0 : 1;
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