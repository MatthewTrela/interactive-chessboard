#ifndef TINY_CHESS_H
#define TINY_CHESS_H

#include <stdbool.h>
#include <stdint.h>

namespace TinyChess {

enum Piece { EMPTY = 0, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum Color { COLOR_WHITE = 0, COLOR_BLACK = 8 };

typedef uint8_t ColoredPiece;

class Game {
   public:
    Game();
    void init();
    void getFEN(char* buffer) const;

    ColoredPiece getPiece(uint8_t sq64) const;
    Color sideToMove() const { return turn; }

    // Fills toSqBuffer64 with legal destination squares. Returns number of legal moves.
    uint8_t getLegalMovesForSquare(uint8_t sq64, uint8_t* toSqBuffer64) const;

    // Validate and Apply
    bool isLegalMove(uint8_t fromSq64, uint8_t toSq64) const;
    void makeMove(uint8_t fromSq64, uint8_t toSq64, uint8_t promotion = QUEEN);

   private:
    ColoredPiece board88[128];
    Color turn;
    uint8_t castling;       // bitmask: 1=WK, 2=WQ, 4=BK, 8=BQ
    uint8_t enPassantSq88;  // 255 if none
    uint16_t halfMoveClock;
    uint16_t fullMove;

    bool isSquareAttacked(const ColoredPiece* b, uint8_t sq88, Color byColor) const;
    void generatePseudoLegal(uint8_t sq88, uint8_t* moves88, uint8_t& count) const;
    bool testMoveLegality(uint8_t from88, uint8_t to88) const;
};

}  // namespace TinyChess
#endif
