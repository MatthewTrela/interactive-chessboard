#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "esp_attr.h"

namespace Chess {

using Bitboard = uint64_t;

enum Square : int {
    SQ_A1 = 0,
    SQ_B1,
    SQ_C1,
    SQ_D1,
    SQ_E1,
    SQ_F1,
    SQ_G1,
    SQ_H1,
    SQ_A2,
    SQ_B2,
    SQ_C2,
    SQ_D2,
    SQ_E2,
    SQ_F2,
    SQ_G2,
    SQ_H2,
    SQ_A3,
    SQ_B3,
    SQ_C3,
    SQ_D3,
    SQ_E3,
    SQ_F3,
    SQ_G3,
    SQ_H3,
    SQ_A4,
    SQ_B4,
    SQ_C4,
    SQ_D4,
    SQ_E4,
    SQ_F4,
    SQ_G4,
    SQ_H4,
    SQ_A5,
    SQ_B5,
    SQ_C5,
    SQ_D5,
    SQ_E5,
    SQ_F5,
    SQ_G5,
    SQ_H5,
    SQ_A6,
    SQ_B6,
    SQ_C6,
    SQ_D6,
    SQ_E6,
    SQ_F6,
    SQ_G6,
    SQ_H6,
    SQ_A7,
    SQ_B7,
    SQ_C7,
    SQ_D7,
    SQ_E7,
    SQ_F7,
    SQ_G7,
    SQ_H7,
    SQ_A8,
    SQ_B8,
    SQ_C8,
    SQ_D8,
    SQ_E8,
    SQ_F8,
    SQ_G8,
    SQ_H8,
    SQ_NONE = 64
};

enum class ChessColor : int { White = 0, Black = 1, None = 2 };

enum class PieceType : int { Pawn = 0, Knight, Bishop, Rook, Queen, King, None };

// Masks used to revoke castling rights when a piece moves to/from a corner or
// king square. Indexed by square; ANDed against the current castling rights
// after every move. Bits: 0=WK-side, 1=WQ-side, 2=BK-side, 3=BQ-side.
const uint8_t castlingRightsMask[64] = {13, 15, 15, 15, 12, 15, 15, 14, 15, 15, 15, 15, 15, 15, 15, 15,
                                        15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                                        15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                                        15, 15, 15, 15, 15, 15, 15, 15, 7,  15, 15, 15, 3,  15, 15, 11};

// Bits 0-5:   From square (0-63)
// Bits 6-11:  To square (0-63)
/* Bits 12-15: Flags
code	promotion	capture	special 1	special 0	kind of move
0	0	0	0	0	quiet moves
1	0	0	0	1	double pawn push
2	0	0	1	0	king castle
3	0	0	1	1	queen castle
4	0	1	0	0	captures
5	0	1	0	1	ep-capture
8	1	0	0	0	knight-promotion
9	1	0	0	1	bishop-promotion
10	1	0	1	0	rook-promotion
11	1	0	1	1	queen-promotion
12	1	1	0	0	knight-promo capture
13	1	1	0	1	bishop-promo capture
14	1	1	1	0	rook-promo capture
15	1	1	1	1	queen-promo capture
*/

/// Encodes a chess move in a single 16-bit integer for fast copying and storage.
class Move {
   public:
    Move() : data(0) {}
    Move(uint16_t d) : data(d) {}

    /// @param from   The square the piece is moving from.
    /// @param to     The square the piece is moving to.
    /// @param flags  Move-type flags (bits 12-15 of the encoding); defaults to
    ///               quiet move (0). See the flag table above for valid values.
    Move(Square from, Square to, uint16_t flags = 0) {
        data = (from & 0x3F) | ((to & 0x3F) << 6) | ((flags & 0x0F) << 12);
    }

    Square getFrom() const { return static_cast<Square>(data & 0x3F); }
    Square getTo() const { return static_cast<Square>((data >> 6) & 0x3F); }
    uint16_t getFlags() const { return (data >> 12) & 0x0F; }

    /// @return True if the move flag has the capture bit set (bit 2 of flags).
    bool isCapture() const { return (getFlags() & 4) != 0; }

    /// @return True if the move flag has the promotion bit set (bit 3 of flags).
    bool isPromotion() const { return (getFlags() & 8) != 0; }

    bool operator==(const Move& other) const { return data == other.data; }

   private:
    uint16_t data;
};

/// Fixed-capacity move list backed by a plain array to avoid heap allocation
/// in performance-critical move generation paths.
class MoveList {
   public:
    void push_back(Move move) { moves[count++] = move; }
    size_t size() const { return count; }
    Move& operator[](size_t index) { return moves[index]; }
    const Move& operator[](size_t index) const { return moves[index]; }

    Move* begin() { return moves; }
    Move* end() { return moves + count; }

   private:
    Move moves[256];
    size_t count = 0;
};

/// Represents a complete chess position using bitboards for efficient move
/// generation. Maintains piece/color/occupancy bitboards, a mailbox for O(1)
/// piece lookup by square, and a StateInfo stack for cheap unmake-move.
class Board {
   public:
    Board() { reset(); }
    ~Board() = default;

    /// Resets the board to the standard starting position.
    void reset();

    /// Parses a FEN string and sets up the board state accordingly.
    /// @param fen  A valid FEN string (e.g. the standard starting-position FEN).
    /// @return     True on success, false if the FEN could not be parsed.
    bool loadFEN(const std::string& fen);

    /// Generates all strictly legal moves for the side to move by filtering
    /// pseudo-legal moves that leave the king in check.
    /// @param moveList  Output list populated with every legal move.
    void generateLegalMoves(MoveList& moveList);

    /// Generates all pseudo-legal moves (moves that are geometrically valid but
    /// may leave the king in check). Placed in IRAM for speed on ESP targets.
    /// @param moveList  Output list populated with pseudo-legal moves.
    IRAM_ATTR void generatePseudoLegalMoves(MoveList& moveList) const;

    /// Applies a move to the board, updating all bitboards, the mailbox,
    /// castling rights, en-passant square, and the Zobrist hash. Saves the
    /// previous state to the history stack so the move can be undone.
    /// @param move  The move to apply. Must be pseudo-legal; legality (king
    ///              safety) is checked internally.
    /// @return      True if the resulting position is legal (king not in check).
    IRAM_ATTR bool makeMove(Move move);

    /// Restores the board to the state it was in before the given move was made,
    /// using the saved StateInfo from the history stack.
    /// @param move  The move to undo. Must be the most recently applied move.
    IRAM_ATTR void unmakeMove(Move move);

    ChessColor getSideToMove() const { return sideToMove; }

    /// @return True if the side to move's king is currently in check.
    bool isCheck() const;

    /// @return True if the side to move has no legal moves and is in check.
    bool isCheckmate() const;

    /// @return True if the side to move has no legal moves but is not in check.
    bool isStalemate() const;

    /// Returns the type of piece occupying a square, ignoring color.
    /// @param sq  The square to query.
    /// @return    The PieceType on that square, or PieceType::None if empty.
    PieceType pieceAt(Square sq) const {
        return (mailbox[sq] < 0) ? PieceType::None : static_cast<PieceType>(mailbox[sq] % 6);
    }

    /// Returns the color of the piece occupying a square.
    /// @param sq  The square to query.
    /// @return    ChessColor::White, ChessColor::Black, or ChessColor::None.
    ChessColor colorAt(Square sq) const;

    /// Returns the Zobrist hash of the current position, suitable for use as
    /// a transposition table key.
    uint64_t getHash() const { return zobristKey; }

    /// Counts the number of leaf nodes at the given depth from the current
    /// position. Used to verify move-generation correctness against known values.
    /// @param depth  The number of plies to search.
    /// @return       The total node count at that depth.
    uint64_t perft(int depth);

    /// @return         Bitboard occupancy private var
    Bitboard getOccupancy();

   private:
    static constexpr int MAX_GAME_PLIES = 256;

    Bitboard pieces[2][6];  // pieces[color][PieceType] — one bitboard per piece type per color
    Bitboard colors[2];     // Union of all piece bitboards for each color
    Bitboard occupancy;     // Union of colors[0] and colors[1]; all occupied squares
    int8_t mailbox[64];     // Encoded piece+color per square; -1 = empty

    ChessColor sideToMove;
    Square enPassantSquare;
    uint8_t castlingRights;

    int halfMoveClock;
    int fullMoveNumber;

    uint64_t zobristKey;

    struct StateInfo {
        Square epSquare;
        uint8_t castlingRights;
        int halfMoveClock;
        PieceType capturedPiece;
        uint64_t zobristKey;
    };

    StateInfo stateHistory[MAX_GAME_PLIES];
    int historyIndex = 0;

    // Encodes color + piece type into a single mailbox byte (0–11).
    int8_t mbEncode(int color, PieceType pt) { return static_cast<int8_t>(color * 6 + static_cast<int>(pt)); }
    int mbColorOf(int8_t v) { return v / 6; }

    /// Rebuilds the mailbox array from the current bitboard state. Used after
    /// loadFEN; makeMove/unmakeMove keep it in sync incrementally instead.
    void rebuildMailbox();

    /// Recomputes the occupancy and colors[] bitboards from pieces[][].
    IRAM_ATTR void updateOccupancy();

    /// Determines whether a given square is attacked by any piece of the
    /// specified color, used for check detection and legal-move filtering.
    /// @param sq       The square to test.
    /// @param byColor  The attacking side.
    /// @return         True if at least one piece of byColor attacks sq.
    IRAM_ATTR bool isSquareAttacked(Square sq, ChessColor byColor) const;
};

namespace BitUtils {
inline void setBit(Bitboard& bb, Square sq) { bb |= (1ULL << sq); }
inline void clearBit(Bitboard& bb, Square sq) { bb &= ~(1ULL << sq); }
inline bool readBit(Bitboard bb, Square sq) { return (bb & (1ULL << sq)) != 0; }

/// Removes and returns the index of the lowest set bit (LSB).
/// Equivalent to trailing-zero count; uses a hardware instruction when
/// available. Modifies the bitboard in place.
/// @param bb  The bitboard to pop from; must be non-zero.
/// @return    The square index of the former LSB.
inline Square popLSB(Bitboard& bb) {
    Square sq = static_cast<Square>(__builtin_ctzll(bb));
    bb &= (bb - 1);
    return sq;
}

/// @param bb  A non-zero bitboard.
/// @return    The square index of the lowest set bit without modifying bb.
inline Square getLSB(Bitboard bb) { return static_cast<Square>(__builtin_ctzll(bb)); }

/// @param bb  A non-zero bitboard.
/// @return    The square index of the highest set bit.
inline Square getMSB(Bitboard bb) { return static_cast<Square>(63 - __builtin_clzll(bb)); }

/// @param bb  Any bitboard.
/// @return    The number of set bits (population count).
inline int countBits(Bitboard bb) { return __builtin_popcountll(bb); }
}  // namespace BitUtils

namespace Attacks {
/// Pre-computes all attack tables (pawns, knights, kings, sliding-piece rays).
/// Must be called once at engine startup before any move generation.
void init();

extern Bitboard pawnAttacks[2][64];  // pawnAttacks[color][square]
extern Bitboard knightAttacks[64];
extern Bitboard kingAttacks[64];

enum Direction { NORTH = 0, SOUTH, EAST, WEST, NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST };

extern Bitboard rays[8][64];  // rays[Direction][square] — full ray bitboard

/// Returns the set of squares a bishop on sq can attack given the current
/// occupancy, using classical ray-casting with o^(o-2r) or a magic table.
/// @param sq         The bishop's square.
/// @param occupancy  Bitboard of all occupied squares (both colors).
/// @return           Bitboard of reachable/attackable squares.
IRAM_ATTR Bitboard getBishopAttacks(Square sq, Bitboard occupancy);

/// Returns the set of squares a rook on sq can attack given the current
/// occupancy.
/// @param sq         The rook's square.
/// @param occupancy  Bitboard of all occupied squares (both colors).
/// @return           Bitboard of reachable/attackable squares.
IRAM_ATTR Bitboard getRookAttacks(Square sq, Bitboard occupancy);

/// Returns bishop attacks OR rook attacks from sq — i.e. all queen moves.
/// @param sq         The queen's square.
/// @param occupancy  Bitboard of all occupied squares (both colors).
/// @return           Bitboard of reachable/attackable squares.
Bitboard getQueenAttacks(Square sq, Bitboard occupancy);
}  // namespace Attacks

}  // namespace Chess