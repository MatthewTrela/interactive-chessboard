#include "chess_state.h"

#include <Arduino.h>

#include "global.h"
#include "led.h"
#include "tiny_chess.h"

// ── Square helpers ──────────────────────────────────────────────────────────
static uint8_t gridToSquare(uint8_t row, uint8_t col) {
    return row * 8 + col;  // 0=A1, 63=H8
}

static void squareToGrid(uint8_t sq, uint8_t& row, uint8_t& col) {
    row = sq / 8;
    col = sq % 8;
}

// ── State ────────────────────────────────────────────────────────────────────
static TinyChess::Game board;
static bool pieceInHand = false;
static uint8_t liftedRow = 0xFF;
static uint8_t liftedCol = 0xFF;
static char fenBuffer[128];

// ── LED feedback helpers ─────────────────────────────────────────────────────
static void highlightLegalMoves(uint8_t fromRow, uint8_t fromCol) {
    uint8_t sq = gridToSquare(fromRow, fromCol);
    uint8_t toMoves[32];
    uint8_t count = board.getLegalMovesForSquare(sq, toMoves);

    for (int i = 0; i < count; i++) {
        uint8_t row, col;
        squareToGrid(toMoves[i], row, col);
        setLED(row, col, 0x0000FF);  // blue = legal destination
    }
    setLED(fromRow, fromCol, 0xFFFF00);  // yellow = piece in hand
}

static void clearHighlights() { syncLEDsFromInputState(); }

// ── Public API ───────────────────────────────────────────────────────────────
void initChessState() {
    board.init();
    pieceInHand = false;
    liftedRow = 0xFF;
    liftedCol = 0xFF;
    Serial.println("[Chess] Initialized to starting position");
    board.getFEN(fenBuffer);
    Serial.println(fenBuffer);
}

MoveResult onSquareEvent(uint8_t row, uint8_t col, SquareEvent event) {
    if (event == SquareEvent::PIECE_LIFTED) {
        uint8_t sq = gridToSquare(row, col);
        TinyChess::ColoredPiece pc = board.getPiece(sq);

        if (pc == TinyChess::EMPTY) return MoveResult::ERROR;

        if ((pc & 8) != board.sideToMove()) {
            Serial.println("[Chess] Wrong turn!");
            return MoveResult::WRONG_TURN;
        }

        pieceInHand = true;
        liftedRow = row;
        liftedCol = col;
        highlightLegalMoves(row, col);
        flushLEDBuffer();
        return MoveResult::AMBIGUOUS;

    } else {  // PIECE_PLACED
        if (!pieceInHand) return MoveResult::ERROR;

        if (row == liftedRow && col == liftedCol) {
            pieceInHand = false;
            liftedRow = 0xFF;
            liftedCol = 0xFF;
            clearHighlights();
            flushLEDBuffer();
            return MoveResult::OK;
        }

        uint8_t from = gridToSquare(liftedRow, liftedCol);
        uint8_t to = gridToSquare(row, col);

        if (!board.isLegalMove(from, to)) {
            Serial.println("[Chess] ILLEGAL move");
            setLED(row, col, 0xFF0000);  // red = illegal
            flushLEDBuffer();
            return MoveResult::ILLEGAL;
        }

        board.makeMove(from, to, TinyChess::QUEEN);  // auto-promote to Queen for now

        pieceInHand = false;
        liftedRow = 0xFF;
        liftedCol = 0xFF;
        clearHighlights();
        flushLEDBuffer();

        board.getFEN(fenBuffer);
        Serial.printf("[Chess] Move applied. FEN: %s\n", fenBuffer);
        return MoveResult::OK;
    }
}

bool isOccupiedByModel(uint8_t row, uint8_t col) { return board.getPiece(gridToSquare(row, col)) != TinyChess::EMPTY; }

const char* getCurrentFEN() {
    if (pieceInHand) return nullptr;
    board.getFEN(fenBuffer);
    return fenBuffer;
}

void resetToStartPosition() {
    board.init();
    pieceInHand = false;
    liftedRow = 0xFF;
    liftedCol = 0xFF;
    clearHighlights();
    flushLEDBuffer();
}
