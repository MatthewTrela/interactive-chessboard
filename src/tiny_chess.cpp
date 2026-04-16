#include "tiny_chess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace TinyChess {

const int dirRook[4] = {16, -16, 1, -1};
const int dirBishop[4] = {17, -17, 15, -15};
const int dirKnight[8] = {33, 31, 18, 14, -33, -31, -18, -14};
const int dirKing[8] = {16, -16, 1, -1, 17, -17, 15, -15};

static inline uint8_t x88(uint8_t sq64) { return sq64 + (sq64 & ~7); }
static inline uint8_t x64(uint8_t sq88) { return sq88 - (sq88 & ~7) / 2; }
static inline bool valid88(int sq88) { return (sq88 & 0x88) == 0; }

Game::Game() { init(); }

void Game::init() {
    memset(board88, 0, sizeof(board88));
    const char* startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
    int sq = 0x70;
    for (int i = 0; startFEN[i]; i++) {
        char c = startFEN[i];
        if (c == '/') {
            sq = (sq - 16) & 0xF0;
        } else if (c >= '1' && c <= '8') {
            sq += (c - '0');
        } else {
            Color color = (c >= 'a' && c <= 'z') ? COLOR_BLACK : COLOR_WHITE;
            char l = (c >= 'a' && c <= 'z') ? c : c + 32;
            int type = EMPTY;
            switch (l) {
                case 'p':
                    type = PAWN;
                    break;
                case 'n':
                    type = KNIGHT;
                    break;
                case 'b':
                    type = BISHOP;
                    break;
                case 'r':
                    type = ROOK;
                    break;
                case 'q':
                    type = QUEEN;
                    break;
                case 'k':
                    type = KING;
                    break;
            }
            board88[sq++] = color | type;
        }
    }
    turn = COLOR_WHITE;
    castling = 15;
    enPassantSq88 = 255;
    halfMoveClock = 0;
    fullMove = 1;
}

ColoredPiece Game::getPiece(uint8_t sq64) const { return board88[x88(sq64)]; }

bool Game::isSquareAttacked(const ColoredPiece* b, uint8_t sq88, Color byColor) const {
    for (int i = 0; i < 8; i++) {
        int nsq = sq88 + dirKnight[i];
        if (valid88(nsq) && b[nsq] == (byColor | KNIGHT)) return true;
    }
    for (int i = 0; i < 8; i++) {
        int nsq = sq88 + dirKing[i];
        if (valid88(nsq) && b[nsq] == (byColor | KING)) return true;
    }
    for (int i = 0; i < 4; i++) {
        int d = dirRook[i];
        int nsq = sq88 + d;
        while (valid88(nsq)) {
            if (b[nsq] != EMPTY) {
                if (b[nsq] == (byColor | ROOK) || b[nsq] == (byColor | QUEEN)) return true;
                break;
            }
            nsq += d;
        }
    }
    for (int i = 0; i < 4; i++) {
        int d = dirBishop[i];
        int nsq = sq88 + d;
        while (valid88(nsq)) {
            if (b[nsq] != EMPTY) {
                if (b[nsq] == (byColor | BISHOP) || b[nsq] == (byColor | QUEEN)) return true;
                break;
            }
            nsq += d;
        }
    }
    int pawnDir = (byColor == COLOR_WHITE) ? -16 : 16;
    int caps[2] = {sq88 + pawnDir - 1, sq88 + pawnDir + 1};
    for (int i = 0; i < 2; i++) {
        if (valid88(caps[i]) && b[caps[i]] == (byColor | PAWN)) return true;
    }
    return false;
}

void Game::generatePseudoLegal(uint8_t sq88, uint8_t* moves88, uint8_t& count) const {
    count = 0;
    ColoredPiece piece = board88[sq88];
    if (piece == EMPTY) return;
    Color color = (Color)(piece & 8);
    int ptype = piece & 7;
    auto addMove = [&](int target) {
        if (board88[target] != EMPTY && (board88[target] & 8) == color) return false;
        moves88[count++] = target;
        return board88[target] == EMPTY;
    };
    if (ptype == PAWN) {
        int dir = (color == COLOR_WHITE) ? 16 : -16;
        int to = sq88 + dir;
        if (valid88(to) && board88[to] == EMPTY) {
            moves88[count++] = to;
            int to2 = to + dir;
            if ((sq88 / 16) == ((color == COLOR_WHITE) ? 1 : 6) && valid88(to2) && board88[to2] == EMPTY)
                moves88[count++] = to2;
        }
        int caps[2] = {sq88 + dir - 1, sq88 + dir + 1};
        for (int i = 0; i < 2; i++) {
            if (valid88(caps[i]) &&
                ((board88[caps[i]] != EMPTY && (board88[caps[i]] & 8) != color) || caps[i] == enPassantSq88))
                moves88[count++] = caps[i];
        }
    } else if (ptype == KNIGHT) {
        for (int i = 0; i < 8; i++) {
            int to = sq88 + dirKnight[i];
            if (valid88(to)) addMove(to);
        }
    } else if (ptype == KING) {
        for (int i = 0; i < 8; i++) {
            int to = sq88 + dirKing[i];
            if (valid88(to)) addMove(to);
        }
        if (color == COLOR_WHITE && sq88 == 0x04) {
            if ((castling & 1) && board88[0x05] == EMPTY && board88[0x06] == EMPTY &&
                !isSquareAttacked(board88, 0x04, COLOR_BLACK) && !isSquareAttacked(board88, 0x05, COLOR_BLACK))
                moves88[count++] = 0x06;
            if ((castling & 2) && board88[0x03] == EMPTY && board88[0x02] == EMPTY && board88[0x01] == EMPTY &&
                !isSquareAttacked(board88, 0x04, COLOR_BLACK) && !isSquareAttacked(board88, 0x03, COLOR_BLACK))
                moves88[count++] = 0x02;
        } else if (color == COLOR_BLACK && sq88 == 0x74) {
            if ((castling & 4) && board88[0x75] == EMPTY && board88[0x76] == EMPTY &&
                !isSquareAttacked(board88, 0x74, COLOR_WHITE) && !isSquareAttacked(board88, 0x75, COLOR_WHITE))
                moves88[count++] = 0x76;
            if ((castling & 8) && board88[0x73] == EMPTY && board88[0x72] == EMPTY && board88[0x71] == EMPTY &&
                !isSquareAttacked(board88, 0x74, COLOR_WHITE) && !isSquareAttacked(board88, 0x73, COLOR_WHITE))
                moves88[count++] = 0x72;
        }
    } else {
        int dirs[8];
        int numDirs = 0;
        if (ptype == ROOK || ptype == QUEEN)
            for (int i = 0; i < 4; i++) dirs[numDirs++] = dirRook[i];
        if (ptype == BISHOP || ptype == QUEEN)
            for (int i = 0; i < 4; i++) dirs[numDirs++] = dirBishop[i];
        for (int i = 0; i < numDirs; i++) {
            int to = sq88 + dirs[i];
            while (valid88(to) && addMove(to)) to += dirs[i];
        }
    }
}

bool Game::testMoveLegality(uint8_t from88, uint8_t to88) const {
    ColoredPiece tempBoard[128];
    memcpy(tempBoard, board88, 128);
    Color myColor = (Color)(tempBoard[from88] & 8);
    int ptype = tempBoard[from88] & 7;
    if (ptype == PAWN && to88 == enPassantSq88) tempBoard[(myColor == COLOR_WHITE) ? to88 - 16 : to88 + 16] = EMPTY;
    if (ptype == KING && abs(from88 - to88) == 2) {
        if (to88 == 0x06) {
            tempBoard[0x05] = tempBoard[0x07];
            tempBoard[0x07] = EMPTY;
        } else if (to88 == 0x02) {
            tempBoard[0x03] = tempBoard[0x00];
            tempBoard[0x00] = EMPTY;
        } else if (to88 == 0x76) {
            tempBoard[0x75] = tempBoard[0x77];
            tempBoard[0x77] = EMPTY;
        } else if (to88 == 0x72) {
            tempBoard[0x73] = tempBoard[0x70];
            tempBoard[0x70] = EMPTY;
        }
    }
    tempBoard[to88] = tempBoard[from88];
    tempBoard[from88] = EMPTY;
    for (int i = 0; i < 128; i++) {
        if (valid88(i) && tempBoard[i] == (myColor | KING))
            return !isSquareAttacked(tempBoard, i, (myColor == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE);
    }
    return false;
}

uint8_t Game::getLegalMovesForSquare(uint8_t sq64, uint8_t* toSqBuffer64) const {
    uint8_t sq88 = x88(sq64), pMoves[32], pCount = 0, lCount = 0;
    generatePseudoLegal(sq88, pMoves, pCount);
    for (int i = 0; i < pCount; i++)
        if (testMoveLegality(sq88, pMoves[i])) toSqBuffer64[lCount++] = x64(pMoves[i]);
    return lCount;
}

bool Game::isLegalMove(uint8_t fromSq64, uint8_t toSq64) const {
    uint8_t moves[32];
    uint8_t count = getLegalMovesForSquare(fromSq64, moves);
    for (int i = 0; i < count; i++)
        if (moves[i] == toSq64) return true;
    return false;
}

void Game::makeMove(uint8_t fromSq64, uint8_t toSq64, uint8_t promotion) {
    uint8_t from88 = x88(fromSq64), to88 = x88(toSq64);
    ColoredPiece mp = board88[from88];
    Color myC = (Color)(mp & 8);
    int ptype = mp & 7;
    halfMoveClock = (ptype == PAWN || board88[to88] != EMPTY) ? 0 : halfMoveClock + 1;
    if (ptype == PAWN && to88 == enPassantSq88) board88[(myC == COLOR_WHITE) ? to88 - 16 : to88 + 16] = EMPTY;
    if (ptype == KING && abs(from88 - to88) == 2) {
        if (to88 == 0x06) {
            board88[0x05] = board88[0x07];
            board88[0x07] = EMPTY;
        } else if (to88 == 0x02) {
            board88[0x03] = board88[0x00];
            board88[0x00] = EMPTY;
        } else if (to88 == 0x76) {
            board88[0x75] = board88[0x77];
            board88[0x77] = EMPTY;
        } else if (to88 == 0x72) {
            board88[0x73] = board88[0x70];
            board88[0x70] = EMPTY;
        }
    }
    board88[to88] = board88[from88];
    board88[from88] = EMPTY;
    if (ptype == PAWN && (to88 / 16 == 0 || to88 / 16 == 7)) board88[to88] = myC | promotion;
    if (from88 == 0x04 || to88 == 0x04) castling &= ~3;
    if (from88 == 0x74 || to88 == 0x74) castling &= ~12;
    if (from88 == 0x00 || to88 == 0x00) castling &= ~2;
    if (from88 == 0x07 || to88 == 0x07) castling &= ~1;
    if (from88 == 0x70 || to88 == 0x70) castling &= ~8;
    if (from88 == 0x77 || to88 == 0x77) castling &= ~4;
    enPassantSq88 = (ptype == PAWN && abs(from88 - to88) == 32) ? (from88 + to88) / 2 : 255;
    if (myC == COLOR_BLACK) fullMove++;
    turn = (myC == COLOR_WHITE) ? COLOR_BLACK : COLOR_WHITE;
}

void Game::getFEN(char* buffer) const {
    int pos = 0;
    for (int rank = 7; rank >= 0; rank--) {
        int emptyCount = 0;
        for (int file = 0; file <= 7; file++) {
            ColoredPiece cp = board88[rank * 16 + file];
            if (cp == EMPTY)
                emptyCount++;
            else {
                if (emptyCount > 0) {
                    buffer[pos++] = '0' + emptyCount;
                    emptyCount = 0;
                }
                char p = "?PNBRQK"[cp & 7];
                if ((cp & 8) == COLOR_WHITE)
                    p += 32;
                else
                    p -= 32;  // Invert purely for FEN correctness
                buffer[pos++] = (cp & 8) == COLOR_WHITE ? p - 32 : p + 32;
            }
        }
        if (emptyCount > 0) buffer[pos++] = '0' + emptyCount;
        if (rank > 0) buffer[pos++] = '/';
    }
    buffer[pos++] = ' ';
    buffer[pos++] = (turn == COLOR_WHITE) ? 'w' : 'b';
    buffer[pos++] = ' ';
    if (castling == 0)
        buffer[pos++] = '-';
    else {
        if (castling & 1) buffer[pos++] = 'K';
        if (castling & 2) buffer[pos++] = 'Q';
        if (castling & 4) buffer[pos++] = 'k';
        if (castling & 8) buffer[pos++] = 'q';
    }
    buffer[pos++] = ' ';
    if (enPassantSq88 == 255)
        buffer[pos++] = '-';
    else {
        buffer[pos++] = 'a' + (enPassantSq88 & 7);
        buffer[pos++] = '1' + (enPassantSq88 / 16);
    }
    sprintf(&buffer[pos], " %d %d", halfMoveClock, fullMove);
}

}  // namespace TinyChess
