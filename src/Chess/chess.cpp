#include "chess.hpp"
#include <cstring>
#include <sstream>

namespace Chess {

namespace Attacks {

    DRAM_ATTR Bitboard pawnAttacks[2][64];
    DRAM_ATTR Bitboard knightAttacks[64];
    DRAM_ATTR Bitboard kingAttacks[64];
    DRAM_ATTR Bitboard rays[8][64];

    constexpr Bitboard NOT_A_FILE  = 0xFEFEFEFEFEFEFEFEULL;
    constexpr Bitboard NOT_AB_FILE = 0xFCFCFCFCFCFCFCFCULL;
    constexpr Bitboard NOT_H_FILE  = 0x7F7F7F7F7F7F7F7FULL;
    constexpr Bitboard NOT_GH_FILE = 0x3F3F3F3F3F3F3F3FULL;

    void init() {
        for (int sq = 0; sq < 64; ++sq) {
            Bitboard bb = (1ULL << sq);

            pawnAttacks[static_cast<int>(ChessColor::White)][sq] = ((bb & NOT_A_FILE) << 7) | ((bb & NOT_H_FILE) << 9);
            pawnAttacks[static_cast<int>(ChessColor::Black)][sq] = ((bb & NOT_H_FILE) >> 7) | ((bb & NOT_A_FILE) >> 9);

            Bitboard l1 = (bb >> 1) & NOT_H_FILE;
            Bitboard l2 = (bb >> 2) & NOT_GH_FILE;
            Bitboard r1 = (bb << 1) & NOT_A_FILE;
            Bitboard r2 = (bb << 2) & NOT_AB_FILE;
            Bitboard h1 = l1 | r1;
            Bitboard h2 = l2 | r2;
            knightAttacks[sq] = (h1 << 16) | (h1 >> 16) | (h2 << 8) | (h2 >> 8);

            Bitboard king_l1 = (bb >> 1) & NOT_H_FILE;
            Bitboard king_r1 = (bb << 1) & NOT_A_FILE;
            Bitboard king_h  = king_l1 | bb | king_r1;
            kingAttacks[sq] = (king_h << 8) | (king_h >> 8) | king_l1 | king_r1;

            int rank = sq / 8;
            int file = sq % 8;

            rays[NORTH][sq] = rays[SOUTH][sq] = rays[EAST][sq] = rays[WEST][sq] = 0;
            rays[NORTH_EAST][sq] = rays[NORTH_WEST][sq] = rays[SOUTH_EAST][sq] = rays[SOUTH_WEST][sq] = 0;

            for (int r = rank + 1; r <= 7; ++r) rays[NORTH][sq] |= (1ULL << (r * 8 + file));
            for (int r = rank - 1; r >= 0; --r) rays[SOUTH][sq] |= (1ULL << (r * 8 + file));
            for (int f = file + 1; f <= 7; ++f) rays[EAST][sq]  |= (1ULL << (rank * 8 + f));
            for (int f = file - 1; f >= 0; --f) rays[WEST][sq]  |= (1ULL << (rank * 8 + f));

            for (int r = rank + 1, f = file + 1; r <= 7 && f <= 7; ++r, ++f) rays[NORTH_EAST][sq] |= (1ULL << (r * 8 + f));
            for (int r = rank + 1, f = file - 1; r <= 7 && f >= 0; ++r, --f) rays[NORTH_WEST][sq] |= (1ULL << (r * 8 + f));
            for (int r = rank - 1, f = file + 1; r >= 0 && f <= 7; --r, ++f) rays[SOUTH_EAST][sq] |= (1ULL << (r * 8 + f));
            for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; --r, --f) rays[SOUTH_WEST][sq] |= (1ULL << (r * 8 + f));
        }
    }

    IRAM_ATTR Bitboard getBishopAttacks(Square sq, Bitboard occ) {
        Bitboard attacks = 0;
        Bitboard blockers;

        blockers = rays[NORTH_EAST][sq] & occ;
        if (blockers) {
            Square blockerSq = BitUtils::getLSB(blockers);
            attacks |= rays[NORTH_EAST][sq] ^ rays[NORTH_EAST][blockerSq];
        } else {
            attacks |= rays[NORTH_EAST][sq];
        }

        blockers = rays[NORTH_WEST][sq] & occ;
        if (blockers) {
            Square blockerSq = BitUtils::getLSB(blockers);
            attacks |= rays[NORTH_WEST][sq] ^ rays[NORTH_WEST][blockerSq];
        } else {
            attacks |= rays[NORTH_WEST][sq];
        }

        blockers = rays[SOUTH_EAST][sq] & occ;
        if (blockers) {
            Square blockerSq = BitUtils::getMSB(blockers);
            attacks |= rays[SOUTH_EAST][sq] ^ rays[SOUTH_EAST][blockerSq];
        } else {
            attacks |= rays[SOUTH_EAST][sq];
        }

        blockers = rays[SOUTH_WEST][sq] & occ;
        if (blockers) {
            Square blockerSq = BitUtils::getMSB(blockers);
            attacks |= rays[SOUTH_WEST][sq] ^ rays[SOUTH_WEST][blockerSq];
        } else {
            attacks |= rays[SOUTH_WEST][sq];
        }

        return attacks;
    }

    IRAM_ATTR Bitboard getRookAttacks(Square sq, Bitboard occ) {
        Bitboard attacks = 0;
        Bitboard blockers;

        blockers = rays[NORTH][sq] & occ;
        if (blockers) {
            Square blockerSq = BitUtils::getLSB(blockers);
            attacks |= rays[NORTH][sq] ^ rays[NORTH][blockerSq];
        } else {
            attacks |= rays[NORTH][sq];
        }

        blockers = rays[EAST][sq] & occ;
        if (blockers) {
            Square blockerSq = BitUtils::getLSB(blockers);
            attacks |= rays[EAST][sq] ^ rays[EAST][blockerSq];
        } else {
            attacks |= rays[EAST][sq];
        }

        blockers = rays[SOUTH][sq] & occ;
        if (blockers) {
            Square blockerSq = BitUtils::getMSB(blockers);
            attacks |= rays[SOUTH][sq] ^ rays[SOUTH][blockerSq];
        } else {
            attacks |= rays[SOUTH][sq];
        }

        blockers = rays[WEST][sq] & occ;
        if (blockers) {
            Square blockerSq = BitUtils::getMSB(blockers);
            attacks |= rays[WEST][sq] ^ rays[WEST][blockerSq];
        } else {
            attacks |= rays[WEST][sq];
        }

        return attacks;
    }

    Bitboard getQueenAttacks(Square sq, Bitboard occ) {
        return getRookAttacks(sq, occ) | getBishopAttacks(sq, occ);
    }
}

void Board::reset() {
    std::memset(pieces, 0, sizeof(pieces));
    std::memset(colors, 0, sizeof(colors));
    occupancy = 0ULL;

    sideToMove = ChessColor::White;
    int w = static_cast<int>(ChessColor::White);
    int b = static_cast<int>(ChessColor::Black);

    pieces[w][static_cast<int>(PieceType::Pawn)] = 0x000000000000FF00ULL;
    pieces[b][static_cast<int>(PieceType::Pawn)] = 0x00FF000000000000ULL;
    pieces[w][static_cast<int>(PieceType::Knight)] = 0x0000000000000042ULL;
    pieces[b][static_cast<int>(PieceType::Knight)] = 0x4200000000000000ULL;
    pieces[w][static_cast<int>(PieceType::Bishop)] = 0x0000000000000024ULL;
    pieces[b][static_cast<int>(PieceType::Bishop)] = 0x2400000000000000ULL;
    pieces[w][static_cast<int>(PieceType::Rook)] = 0x0000000000000081ULL;
    pieces[b][static_cast<int>(PieceType::Rook)] = 0x8100000000000000ULL;
    pieces[w][static_cast<int>(PieceType::Queen)] = 0x0000000000000008ULL;
    pieces[b][static_cast<int>(PieceType::Queen)] = 0x0800000000000000ULL;
    pieces[w][static_cast<int>(PieceType::King)] = 0x0000000000000010ULL;
    pieces[b][static_cast<int>(PieceType::King)] = 0x1000000000000000ULL;

    updateOccupancy();
    rebuildMailbox();

    enPassantSquare = SQ_NONE;
    castlingRights = 0b1111;
    halfMoveClock = 0;
    fullMoveNumber = 1;
    zobristKey = 0ULL;
    
    historyIndex = 0;
}

IRAM_ATTR void Board::updateOccupancy() {
    colors[static_cast<int>(ChessColor::White)] = pieces[0][0] | pieces[0][1] | pieces[0][2] | pieces[0][3] | pieces[0][4] | pieces[0][5];
    colors[static_cast<int>(ChessColor::Black)] = pieces[1][0] | pieces[1][1] | pieces[1][2] | pieces[1][3] | pieces[1][4] | pieces[1][5];
    occupancy = colors[0] | colors[1];
}

bool Board::loadFEN(const std::string& fen) {
    std::memset(pieces, 0, sizeof(pieces));
    std::memset(colors, 0, sizeof(colors));
    occupancy = 0ULL;
    historyIndex = 0;

    std::istringstream ss(fen);
    std::string token;

    ss >> token; 
    int row = 7;
    int col = 0;

    for (char c : token) {
        if (c == '/') {
            row--;
            col = 0;
        } else if (std::isdigit(c)) {
            col += (c - '0'); 
        } else {
            int color = std::islower(c) ? static_cast<int>(ChessColor::Black) : static_cast<int>(ChessColor::White);
            int type = static_cast<int>(PieceType::None);

            switch (std::tolower(c)) {
                case 'p': type = static_cast<int>(PieceType::Pawn); break;
                case 'n': type = static_cast<int>(PieceType::Knight); break;
                case 'b': type = static_cast<int>(PieceType::Bishop); break;
                case 'r': type = static_cast<int>(PieceType::Rook); break;
                case 'q': type = static_cast<int>(PieceType::Queen); break;
                case 'k': type = static_cast<int>(PieceType::King); break;
            }

            if (type != static_cast<int>(PieceType::None)) {
                int sq = row * 8 + col;
                pieces[color][type] |= (1ULL << sq);
                col++;
            }
        }
    }

    ss >> token;
    sideToMove = (token == "w") ? ChessColor::White : ChessColor::Black;

    ss >> token;
    castlingRights = 0;
    if (token != "-") {
        for (char c : token) {
            if (c == 'K') castlingRights |= 1;
            if (c == 'Q') castlingRights |= 2;
            if (c == 'k') castlingRights |= 4;
            if (c == 'q') castlingRights |= 8;
        }
    }

    ss >> token;
    if (token == "-") {
        enPassantSquare = SQ_NONE;
    } else {
        int epFile = token[0] - 'a';
        int epRank = token[1] - '1';
        enPassantSquare = static_cast<Square>(epRank * 8 + epFile);
    }

    if (ss >> halfMoveClock) {
        ss >> fullMoveNumber;
    } else {
        halfMoveClock = 0;
        fullMoveNumber = 1;
    }

    updateOccupancy();
    rebuildMailbox();

    return true;
}

ChessColor Board::colorAt(Square sq) const {
    if (BitUtils::readBit(colors[static_cast<int>(ChessColor::White)], sq)) {
        return ChessColor::White;
    }

    if (BitUtils::readBit(colors[static_cast<int>(ChessColor::Black)], sq)) {
        return ChessColor::Black;
    }
    
    return ChessColor::None; 
}

void Board::rebuildMailbox() {
    std::memset(mailbox, -1, sizeof(mailbox));
    for (int c = 0; c < 2; ++c)
        for (int p = 0; p < 6; ++p) {
            Bitboard bb = pieces[c][p];
            while (bb) {
                Square sq = BitUtils::popLSB(bb);
                mailbox[sq] = mbEncode(c, static_cast<PieceType>(p));
            }
        }
}

IRAM_ATTR bool Board::isSquareAttacked(Square sq, ChessColor byColor) const {
    int attacker = static_cast<int>(byColor);
    Bitboard occ = occupancy;
    
    if (Attacks::knightAttacks[sq] & pieces[attacker][1]) return true;
    if (Attacks::kingAttacks[sq] & pieces[attacker][5]) return true;
    if (Attacks::pawnAttacks[attacker ^ 1][sq] & pieces[attacker][0]) return true;
    
    Bitboard queens = pieces[attacker][4];
    
    Bitboard bishops = pieces[attacker][2] | queens;
    if (bishops && (Attacks::getBishopAttacks(sq, occ) & bishops)) return true;
    
    Bitboard rooks = pieces[attacker][3] | queens;
    if (rooks && (Attacks::getRookAttacks(sq, occ) & rooks)) return true;
    
    return false;
}

bool Board::isCheck() const {
    int turn = static_cast<int>(sideToMove);
    return isSquareAttacked(BitUtils::getLSB(pieces[turn][static_cast<int>(PieceType::King)]), static_cast<ChessColor>(turn ^ 1));
}

void Board::generateLegalMoves(MoveList& moveList) {
    MoveList pseudoMoves;
    generatePseudoLegalMoves(pseudoMoves);

    for (size_t i = 0; i < pseudoMoves.size(); ++i) {
        Move move = pseudoMoves[i];
        if (makeMove(move)) {
            moveList.push_back(move);  // ← remove the isSquareAttacked() call
            unmakeMove(move);
        }
    }
}

IRAM_ATTR void Board::generatePseudoLegalMoves(MoveList& moveList) const {
    Bitboard empty = ~occupancy;
    int us = static_cast<int>(sideToMove);
    int them = us ^ 1;
    Bitboard enemies = colors[them];
    Bitboard validSquares = ~(colors[us]);

    Bitboard pawns = pieces[us][static_cast<int>(PieceType::Pawn)];

    if (sideToMove == ChessColor::White) {
        Bitboard singlePushes = (pawns << 8) & empty;
        Bitboard loopB = singlePushes;
        
        while (loopB) {
            Square to = BitUtils::popLSB(loopB);
            Square from = static_cast<Square>(to - 8);
            
            if (to >= SQ_A8) {
                moveList.push_back(Move(from, to, 11));
                moveList.push_back(Move(from, to, 8));
                moveList.push_back(Move(from, to, 10));
                moveList.push_back(Move(from, to, 9));
            } else {
                moveList.push_back(Move(from, to, 0));
            }
        }

        Bitboard doublePushes = ((singlePushes & 0x0000000000FF0000ULL) << 8) & empty;
        loopB = doublePushes;
        
        while (loopB) {
            Square to = BitUtils::popLSB(loopB);
            Square from = static_cast<Square>(to - 16);
            moveList.push_back(Move(from, to, 1));
        }

        Bitboard leftCaps = ((pawns & Attacks::NOT_A_FILE) << 7) & enemies;
        Bitboard rightCaps = ((pawns & Attacks::NOT_H_FILE) << 9) & enemies;

        loopB = leftCaps;
        while (loopB) {
            Square to = BitUtils::popLSB(loopB);
            Square from = static_cast<Square>(to - 7);
            if (to >= SQ_A8) {
                moveList.push_back(Move(from, to, 15));
                moveList.push_back(Move(from, to, 12));
                moveList.push_back(Move(from, to, 14));
                moveList.push_back(Move(from, to, 13));
            } else {
                moveList.push_back(Move(from, to, 4));
            }
        }

        loopB = rightCaps;
        while (loopB) {
            Square to = BitUtils::popLSB(loopB);
            Square from = static_cast<Square>(to - 9);
            if (to >= SQ_A8) {
                moveList.push_back(Move(from, to, 15)); 
                moveList.push_back(Move(from, to, 12)); 
                moveList.push_back(Move(from, to, 14)); 
                moveList.push_back(Move(from, to, 13)); 
            } else {
                moveList.push_back(Move(from, to, 4));  
            }
        }

        if (enPassantSquare != SQ_NONE) {
            Bitboard epBB = (1ULL << enPassantSquare);
            Bitboard epLeft = ((pawns & Attacks::NOT_A_FILE) << 7) & epBB;
            Bitboard epRight = ((pawns & Attacks::NOT_H_FILE) << 9) & epBB;

            if (epLeft) {
                Square from = static_cast<Square>(enPassantSquare - 7);
                moveList.push_back(Move(from, enPassantSquare, 5));
            }
            if (epRight) {
                Square from = static_cast<Square>(enPassantSquare - 9);
                moveList.push_back(Move(from, enPassantSquare, 5));
            }
        }

        if (castlingRights & 1) { 
            Bitboard pathMask = (1ULL << SQ_F1) | (1ULL << SQ_G1);
            if ((occupancy & pathMask) == 0) {
                if (!isSquareAttacked(SQ_E1, ChessColor::Black) && 
                    !isSquareAttacked(SQ_F1, ChessColor::Black) && 
                    !isSquareAttacked(SQ_G1, ChessColor::Black)) {
                    moveList.push_back(Move(SQ_E1, SQ_G1, 2));
                }
            }
        }

        if (castlingRights & 2) {
            Bitboard pathMask = (1ULL << SQ_B1) | (1ULL << SQ_C1) | (1ULL << SQ_D1);
            if ((occupancy & pathMask) == 0) {
                if (!isSquareAttacked(SQ_E1, ChessColor::Black) && 
                    !isSquareAttacked(SQ_D1, ChessColor::Black) && 
                    !isSquareAttacked(SQ_C1, ChessColor::Black)) {
                    moveList.push_back(Move(SQ_E1, SQ_C1, 3));
                }
            }
        }
    } else {
        Bitboard singlePushes = (pawns >> 8) & empty;
        Bitboard loopB = singlePushes;
        
        while (loopB) {
            Square to = BitUtils::popLSB(loopB);
            Square from = static_cast<Square>(to + 8);
            
            if (to <= SQ_H1) { 
                moveList.push_back(Move(from, to, 11));
                moveList.push_back(Move(from, to, 8));
                moveList.push_back(Move(from, to, 10));
                moveList.push_back(Move(from, to, 9));
            } else {
                moveList.push_back(Move(from, to, 0));
            }
        }

        Bitboard doublePushes = ((singlePushes & 0x0000FF0000000000ULL) >> 8) & empty;
        loopB = doublePushes;
        
        while (loopB) {
            Square to = BitUtils::popLSB(loopB);
            Square from = static_cast<Square>(to + 16);
            moveList.push_back(Move(from, to, 1));
        }

        Bitboard leftCaps = ((pawns & Attacks::NOT_A_FILE) >> 9) & enemies;
        Bitboard rightCaps = ((pawns & Attacks::NOT_H_FILE) >> 7) & enemies;

        loopB = leftCaps;
        while (loopB) {
            Square to = BitUtils::popLSB(loopB);
            Square from = static_cast<Square>(to + 9); // Math inverted to +9
            if (to <= SQ_H1) {
                moveList.push_back(Move(from, to, 15));
                moveList.push_back(Move(from, to, 12));
                moveList.push_back(Move(from, to, 14));
                moveList.push_back(Move(from, to, 13));
            } else {
                moveList.push_back(Move(from, to, 4));
            }
        }

        loopB = rightCaps;
        while (loopB) {
            Square to = BitUtils::popLSB(loopB);
            Square from = static_cast<Square>(to + 7);
            if (to <= SQ_H1) {
                moveList.push_back(Move(from, to, 15)); 
                moveList.push_back(Move(from, to, 12)); 
                moveList.push_back(Move(from, to, 14)); 
                moveList.push_back(Move(from, to, 13)); 
            } else {
                moveList.push_back(Move(from, to, 4));  
            }
        }

        if (enPassantSquare != SQ_NONE) {
            Bitboard epBB = (1ULL << enPassantSquare);
            Bitboard epLeft = ((pawns & Attacks::NOT_A_FILE) >> 9) & epBB;
            Bitboard epRight = ((pawns & Attacks::NOT_H_FILE) >> 7) & epBB;

            if (epLeft) {
                Square from = static_cast<Square>(enPassantSquare + 9);
                moveList.push_back(Move(from, enPassantSquare, 5));
            }
            if (epRight) {
                Square from = static_cast<Square>(enPassantSquare + 7);
                moveList.push_back(Move(from, enPassantSquare, 5));
            }
        }

        if (castlingRights & 4) { 
            Bitboard pathMask = (1ULL << SQ_F8) | (1ULL << SQ_G8);
            if ((occupancy & pathMask) == 0) {
                if (!isSquareAttacked(SQ_E8, ChessColor::White) && 
                    !isSquareAttacked(SQ_F8, ChessColor::White) && 
                    !isSquareAttacked(SQ_G8, ChessColor::White)) {
                    
                    moveList.push_back(Move(SQ_E8, SQ_G8, 2));
                }
            }
        }

        if (castlingRights & 8) { 
            Bitboard pathMask = (1ULL << SQ_B8) | (1ULL << SQ_C8) | (1ULL << SQ_D8);
            if ((occupancy & pathMask) == 0) {
                if (!isSquareAttacked(SQ_E8, ChessColor::White) && 
                    !isSquareAttacked(SQ_D8, ChessColor::White) && 
                    !isSquareAttacked(SQ_C8, ChessColor::White)) {
                    
                    moveList.push_back(Move(SQ_E8, SQ_C8, 3));
                }
            }
        }
    }
    
    Bitboard knights = pieces[us][static_cast<int>(PieceType::Knight)];
    while (knights) {
        Square from = BitUtils::popLSB(knights);
        Bitboard attacks = Attacks::knightAttacks[from] & validSquares;
        
        while (attacks) {
            Square to = BitUtils::popLSB(attacks);
            uint16_t flag = ((1ULL << to) & enemies) ? 4 : 0; 
            moveList.push_back(Move(from, to, flag));
        }
    }

    Bitboard kings = pieces[us][static_cast<int>(PieceType::King)];
    while (kings) {
        Square from = BitUtils::popLSB(kings);
        Bitboard attacks = Attacks::kingAttacks[from] & validSquares;
        
        while (attacks) {
            Square to = BitUtils::popLSB(attacks);
            uint16_t flag = ((1ULL << to) & enemies) ? 4 : 0;
            moveList.push_back(Move(from, to, flag));
        }
    }

    Bitboard diagonalSliders = pieces[us][static_cast<int>(PieceType::Bishop)] | pieces[us][static_cast<int>(PieceType::Queen)];
    while (diagonalSliders) {
        Square from = BitUtils::popLSB(diagonalSliders);
        Bitboard attacks = Attacks::getBishopAttacks(from, occupancy) & validSquares;
        
        while (attacks) {
            Square to = BitUtils::popLSB(attacks);
            uint16_t flag = ((1ULL << to) & enemies) ? 4 : 0;
            moveList.push_back(Move(from, to, flag));
        }
    }

    Bitboard straightSliders = pieces[us][static_cast<int>(PieceType::Rook)] | pieces[us][static_cast<int>(PieceType::Queen)];
    while (straightSliders) {
        Square from = BitUtils::popLSB(straightSliders);
        Bitboard attacks = Attacks::getRookAttacks(from, occupancy) & validSquares;

        while (attacks) {
            Square to = BitUtils::popLSB(attacks);
            uint16_t flag = ((1ULL << to) & enemies) ? 4 : 0;
            moveList.push_back(Move(from, to, flag));
        }
    }

}

IRAM_ATTR bool Board::makeMove(Move move) {
    int us = static_cast<int>(sideToMove);
    int them = us ^ 1;
    Square from = move.getFrom();
    Square to = move.getTo();
    uint16_t flags = move.getFlags();

    PieceType movedPiece = pieceAt(from);
    PieceType capturedPiece = PieceType::None;
    
    if (move.isCapture()) {
        if (flags == 5) {
            capturedPiece = PieceType::Pawn;
        } else {
            capturedPiece = pieceAt(to);
        }
    }

    StateInfo state;
    state.epSquare = enPassantSquare;
    state.castlingRights = castlingRights;
    state.halfMoveClock = halfMoveClock;
    state.capturedPiece = capturedPiece;
    state.zobristKey = zobristKey;
    stateHistory[historyIndex++] = state;

    BitUtils::clearBit(pieces[us][static_cast<int>(movedPiece)], from);
    BitUtils::setBit(pieces[us][static_cast<int>(movedPiece)], to);

    if (capturedPiece != PieceType::None) {
        if (flags == 5) {
            Square cap = (sideToMove == ChessColor::White) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8);
            BitUtils::clearBit(pieces[them][static_cast<int>(PieceType::Pawn)], cap);
            mailbox[cap] = -1;
        } else {
            BitUtils::clearBit(pieces[them][static_cast<int>(capturedPiece)], to);
        }
    }

    if (flags >= 8) {
        BitUtils::clearBit(pieces[us][static_cast<int>(PieceType::Pawn)], to);
        int promo = (flags & 3) + 1; 
        BitUtils::setBit(pieces[us][promo], to);
        mailbox[from] = -1;         
        mailbox[to] = mbEncode(us, static_cast<PieceType>(promo));
    } else {
        mailbox[to]   = mailbox[from];
        mailbox[from] = -1;
    }

    if (flags == 2) {
        Square rookFrom = (sideToMove == ChessColor::White) ? SQ_H1 : SQ_H8;
        Square rookTo   = (sideToMove == ChessColor::White) ? SQ_F1 : SQ_F8;
        BitUtils::clearBit(pieces[us][static_cast<int>(PieceType::Rook)], rookFrom);
        BitUtils::setBit(pieces[us][static_cast<int>(PieceType::Rook)], rookTo);
        mailbox[rookTo]   = mbEncode(us, PieceType::Rook);
        mailbox[rookFrom] = -1;
    } else if (flags == 3) {
        Square rookFrom = (sideToMove == ChessColor::White) ? SQ_A1 : SQ_A8;
        Square rookTo   = (sideToMove == ChessColor::White) ? SQ_D1 : SQ_D8;
        BitUtils::clearBit(pieces[us][static_cast<int>(PieceType::Rook)], rookFrom);
        BitUtils::setBit(pieces[us][static_cast<int>(PieceType::Rook)], rookTo);
        mailbox[rookTo] = mbEncode(us, PieceType::Rook);
        mailbox[rookFrom] = -1;
    }

    enPassantSquare = SQ_NONE;
    if (flags == 1) {
        enPassantSquare = (sideToMove == ChessColor::White) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8);
    }
    
    castlingRights &= castlingRightsMask[from];
    castlingRights &= castlingRightsMask[to];

    if (movedPiece == PieceType::Pawn || capturedPiece != PieceType::None) {
        halfMoveClock = 0;
    } else {
        halfMoveClock++;
    }

    if (sideToMove == ChessColor::Black) fullMoveNumber++;
    
    updateOccupancy();
    sideToMove = static_cast<ChessColor>(them);

    Square kingSq = BitUtils::getLSB(pieces[us][static_cast<int>(PieceType::King)]);
    if (isSquareAttacked(kingSq, sideToMove)) {
        unmakeMove(move);
        return false; 
    }

    return true;
}

IRAM_ATTR void Board::unmakeMove(Move move) {
    sideToMove = (sideToMove == ChessColor::White) ? ChessColor::Black : ChessColor::White;
    int us = static_cast<int>(sideToMove);
    int them = us ^ 1;

    Square from = move.getFrom();
    Square to = move.getTo();
    uint16_t flags = move.getFlags();

    StateInfo state = stateHistory[--historyIndex];

    enPassantSquare = state.epSquare;
    castlingRights = state.castlingRights;
    halfMoveClock = state.halfMoveClock;
    zobristKey = state.zobristKey;

    if (sideToMove == ChessColor::Black) fullMoveNumber--;
    PieceType movedPiece = (flags >= 8) ? PieceType::Pawn : pieceAt(to);
    
    if (flags >= 8) {
        int promoTypeIndex = (flags & 3) + 1;
        BitUtils::clearBit(pieces[us][promoTypeIndex], to);
    } else {
        BitUtils::clearBit(pieces[us][static_cast<int>(movedPiece)], to);
    }
    
    BitUtils::setBit(pieces[us][static_cast<int>(movedPiece)], from);

    mailbox[from] = mbEncode(us, movedPiece);
    mailbox[to]   = -1;

    if (state.capturedPiece != PieceType::None) {
        if (flags == 5) {
            Square cap = (sideToMove == ChessColor::White) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8);
            BitUtils::setBit(pieces[them][static_cast<int>(PieceType::Pawn)], cap);
            mailbox[cap] = mbEncode(them, PieceType::Pawn);
        } else {
            BitUtils::setBit(pieces[them][static_cast<int>(state.capturedPiece)], to);
            mailbox[to] = mbEncode(them, state.capturedPiece);
        }
    }

    if (flags == 2) { 
        Square rookFrom = (sideToMove == ChessColor::White) ? SQ_H1 : SQ_H8;
        Square rookTo   = (sideToMove == ChessColor::White) ? SQ_F1 : SQ_F8;
        BitUtils::clearBit(pieces[us][static_cast<int>(PieceType::Rook)], rookTo);
        BitUtils::setBit(pieces[us][static_cast<int>(PieceType::Rook)], rookFrom);
        mailbox[rookFrom] = mbEncode(us, PieceType::Rook);
        mailbox[rookTo]   = -1;
    } else if (flags == 3) { 
        Square rookFrom = (sideToMove == ChessColor::White) ? SQ_A1 : SQ_A8;
        Square rookTo   = (sideToMove == ChessColor::White) ? SQ_D1 : SQ_D8;
        BitUtils::clearBit(pieces[us][static_cast<int>(PieceType::Rook)], rookTo);
        BitUtils::setBit(pieces[us][static_cast<int>(PieceType::Rook)], rookFrom);
        mailbox[rookFrom] = mbEncode(us, PieceType::Rook);
        mailbox[rookTo]   = -1;
    }

    updateOccupancy();
}

bool Board::isCheckmate() const {
    MoveList moves;
    const_cast<Board*>(this)->generateLegalMoves(moves);
    return moves.size() == 0 && isCheck();
}

bool Board::isStalemate() const {
    MoveList moves;
    const_cast<Board*>(this)->generateLegalMoves(moves);
    return moves.size() == 0 && !isCheck();
}

uint64_t Board::perft(int depth) {
    if (depth == 0) return 1ULL;

    MoveList pseudoMoves;
    generatePseudoLegalMoves(pseudoMoves);

    if (depth == 1) {
        uint64_t nodes = 0;
        for (size_t i = 0; i < pseudoMoves.size(); ++i)
            if (makeMove(pseudoMoves[i])) { ++nodes; unmakeMove(pseudoMoves[i]); }
        return nodes;
    }

    uint64_t nodes = 0;
    for (size_t i = 0; i < pseudoMoves.size(); ++i)
        if (makeMove(pseudoMoves[i])) { nodes += perft(depth - 1); unmakeMove(pseudoMoves[i]); }
    return nodes;
}

}