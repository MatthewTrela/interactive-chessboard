#include "engine.h"
#include <Arduino.h>
#include <string.h>

namespace ChessEngine {

    // Timing and search control
    static uint32_t searchTimeLimit = 1000;
    static unsigned long startTime = 0;
    static unsigned long lastYieldTime = 0;
    static bool timeOut = false;
    static bool forceAbort = false;
    static uint32_t nodes = 0;

    const int MATE_SCORE = 30000;
    const int DRAW_SCORE = 0;

    void setTimeLimit(uint32_t ms) { searchTimeLimit = ms; }
    void abortSearch() { forceAbort = true;}

    inline void checkTime() {
        if ((nodes & 2047) == 0) {
            unsigned long now = millis();
            if (now - startTime >= searchTimeLimit || forceAbort)
                timeOut = true;
            if (now - lastYieldTime > 50) {
                vTaskDelay(pdMS_TO_TICKS(1));
                lastYieldTime = millis();
            }
        }
    }

    // Piece values (separate midgame and endgame)
    static const int MG_VALUE[6] = {82, 337, 365, 477, 1025, 0};
    static const int EG_VALUE[6] = {94, 281, 297, 512,  936, 0};
    static const int PHASE_INC[6] = {0, 1, 1, 2, 4,0};
    static const int MAX_PHASE = 24;

    static inline int pstIndex(int sq, bool isWhite) {
        return isWhite ? (7 - (sq >> 3)) * 8 + (sq & 7) : sq;
    }

    // Piece-Square Tables
    static const int PST_PAWN_MG[64] = {
          0,   0,   0,   0,   0,   0,   0,   0,
         98, 134,  61,  95,  68, 126,  34, -11,
         -6,   7,  26,  31,  65,  56,  25, -20,
        -14,  13,   6,  21,  23,  12,  17, -23,
        -27,  -2,  -5,  12,  17,   6,  10, -25,
        -26,  -4,  -4, -10,   3,   3,  33, -12,
        -35,  -1, -20, -23, -15,  24,  38, -22,
          0,   0,   0,   0,   0,   0,   0,   0
    };
    static const int PST_PAWN_EG[64] = {
          0,   0,   0,   0,   0,   0,   0,   0,
        178, 173, 158, 134, 147, 132, 165, 187,
         94, 100,  85,  67,  56,  53,  82,  84,
         32,  24,  13,   5,  -2,   4,  17,  17,
         13,   9,  -3,  -7,  -7,  -8,   3,  -1,
          4,   7,  -6,   1,   0,  -5,  -1,  -8,
         13,   8,   8,  10,  13,   0,   2,  -7,
          0,   0,   0,   0,   0,   0,   0,   0
    };
    static const int PST_KNIGHT_MG[64] = {
        -167, -89, -34, -49,  61, -97, -15,-107,
         -73, -41,  72,  36,  23,  62,   7, -17,
         -47,  60,  37,  65,  84, 129,  73,  44,
          -9,  17,  19,  53,  37,  69,  18,  22,
         -13,   4,  16,  13,  28,  19,  21,  -8,
         -23,  -9,  12,  10,  19,  17,  25, -16,
         -29, -53, -12,  -3,  -1,  18, -14, -19,
        -105, -21, -58, -33, -17, -28, -19, -23
    };
    static const int PST_KNIGHT_EG[64] = {
         -58, -38, -13, -28, -31, -27, -63, -99,
         -25,  -8, -25,  -2,  -9, -25, -24, -52,
         -24, -20,  10,   9,  -1,  -9, -19, -41,
         -17,   3,  22,  22,  22,  11,   8, -18,
         -18,  -6,  16,  25,  16,  17,   4, -18,
         -23,  -3,  -1,  15,  10,  -3, -20, -22,
         -42, -20, -10,  -5,  -2, -20, -23, -44,
         -29, -51, -23, -15, -22, -18, -50, -64
    };
    static const int PST_BISHOP_MG[64] = {
         -29,   4, -82, -37, -25, -42,   7,  -8,
         -26,  16, -18, -13,  30,  59,  18, -47,
         -16,  37,  43,  40,  35,  50,  37,  -2,
          -4,   5,  19,  50,  37,  37,   7,  -2,
          -6,  13,  13,  26,  34,  12,  10,   4,
           0,  15,  15,  15,  14,  27,  18,  10,
           4,  15,  16,   0,   7,  21,  33,   1,
         -33,  -3, -14, -21, -13, -12, -39, -21
    };
    static const int PST_BISHOP_EG[64] = {
         -14, -21, -11,  -8,  -7,  -9, -17, -24,
          -8,  -4,   7, -12,  -3, -13,  -4, -14,
           2,  -8,   0,  -1,  -2,   6,   0,   4,
          -3,   9,  12,   9,  14,  10,   3,   2,
          -6,   3,  13,  19,   7,  10,  -3,  -9,
         -12,  -3,   8,  10,  13,   3,  -7, -15,
         -14, -18,  -7,  -1,   4,  -9, -15, -27,
         -23,  -9, -23,  -5,  -9, -16,  -5, -17
    };
    static const int PST_ROOK_MG[64] = {
          32,  42,  32,  51,  63,   9,  31,  43,
          27,  32,  58,  62,  80,  67,  26,  44,
          -5,  19,  26,  36,  17,  45,  61,  16,
         -24, -11,   7,  26,  24,  35,  -8, -20,
         -36, -26, -12,  -1,   9,  -7,   6, -23,
         -45, -25, -16, -17,   3,   0,  -5, -33,
         -44, -16, -20,  -9,  -1,  11,  -6, -71,
         -19, -13,   1,  17,  16,   7, -37, -26
    };
    static const int PST_ROOK_EG[64] = {
          13,  10,  18,  15,  12,  12,   8,   5,
          11,  13,  13,  11,  -3,   3,   8,   3,
           7,   7,   7,   5,   4,  -3,  -5,  -3,
           4,   3,  13,   1,   2,   1,  -1,   2,
           3,   5,   8,   4,  -5,  -6,  -8, -11,
          -4,   0,  -5,  -1,  -7, -12,  -8, -16,
          -6,  -6,   0,   2,  -9,  -9, -11,  -3,
          -9,   2,   3,  -1,  -5, -13,   4, -20
    };
    static const int PST_QUEEN_MG[64] = {
         -28,   0,  29,  12,  59,  44,  43,  45,
         -24, -39,  -5,   1, -16,  57,  28,  54,
         -13, -17,   7,   8,  29,  56,  47,  57,
         -27, -27, -16, -16,  -1,  17,  -2,   1,
          -9, -26,  -9, -10,  -2,  -4,   3,  -3,
         -14,   2, -11,  -2,  -5,   2,  14,   5,
         -35,  -8,  11,   2,   8,  15,  -3,   1,
          -1, -18,  -9,  10, -15, -25, -31, -50
    };
    static const int PST_QUEEN_EG[64] = {
          -9,  22,  22,  27,  27,  19,  10,  20,
         -17,  20,  32,  41,  58,  25,  30,   0,
         -20,   6,   9,  49,  47,  35,  19,   9,
           3,  22,  24,  45,  57,  40,  57,  36,
         -18,  28,  19,  47,  31,  34,  39,  23,
         -16, -27,  15,   6,   9,  17,  10,   5,
         -22, -23, -30, -16, -16, -23, -36, -32,
         -33, -28, -22, -43,  -5, -32, -20, -41
    };
    static const int PST_KING_MG[64] = {
         -65,  23,  16, -15, -56, -34,   2,  13,
          29,  -1, -20,  -7,  -8,  -4, -38, -29,
          -9,  24,   2, -16, -20,   6,  22, -22,
         -17, -20, -12, -27, -30, -25, -14, -36,
         -49,  -1, -27, -39, -46, -44, -33, -51,
         -14, -14, -22, -46, -44, -30, -15, -27,
           1,   7,  -8, -64, -43, -16,   9,   8,
         -15,  36,  12, -54,   8, -28,  24,  14
    };
    static const int PST_KING_EG[64] = {
         -74, -35, -18, -18, -11,  15,   4, -17,
         -12,  17,  14,  17,  17,  38,  23,  11,
          10,  17,  23,  15,  20,  45,  44,  13,
          -8,  22,  24,  27,  26,  33,  26,   3,
         -18,  -4,  21,  24,  27,  23,   9, -11,
         -19,  -3,  11,  21,  23,  16,   7,  -9,
         -27, -11,   4,  13,  14,   4,  -5, -17,
         -53, -34, -21, -11, -28, -14, -24, -43
    };

    static inline int getPST_MG(int pt, int idx) {
        switch (pt) {
            case 0:  return PST_PAWN_MG[idx];
            case 1:  return PST_KNIGHT_MG[idx];
            case 2:  return PST_BISHOP_MG[idx];
            case 3:  return PST_ROOK_MG[idx];
            case 4:  return PST_QUEEN_MG[idx];
            default: return PST_KING_MG[idx];
        }
    }
    static inline int getPST_EG(int pt, int idx) {
        switch (pt) {
            case 0:  return PST_PAWN_EG[idx];
            case 1:  return PST_KNIGHT_EG[idx];
            case 2:  return PST_BISHOP_EG[idx];
            case 3:  return PST_ROOK_EG[idx];
            case 4:  return PST_QUEEN_EG[idx];
            default: return PST_KING_EG[idx];
        }
    }

    // Move-ordering tables
    static Chess::Move killerMoves[2][64];
    static int historyTable[64][64];

    // MVV-LVA
    static const int MVV_LVA[36] = {
        15, 14, 13, 12, 11, 10,
        25, 24, 23, 22, 21, 20,
        35, 34, 33, 32, 31, 30,
        45, 44, 43, 42, 41, 40,
        55, 54, 53, 52, 51, 50,
         0,  0,  0,  0,  0,  0
    };

    static int scoreMoveForOrdering(const Chess::Move& m,const Chess::Board& board,int ply) {
        if (m.isCapture()) {
            Chess::PieceType att = board.pieceAt(m.getFrom());
            Chess::PieceType vic = (m.getFlags() == 5) ? Chess::PieceType::Pawn : board.pieceAt(m.getTo());
            if (att != Chess::PieceType::None && vic != Chess::PieceType::None) {
                int vi = static_cast<int>(vic);
                int ai = static_cast<int>(att);
                return 10000 + MVV_LVA[vi * 6 + ai];
            }
        }
        if (m.isPromotion()) return 9000;
        if (m == killerMoves[0][ply]) return 8000;
        if (m == killerMoves[1][ply]) return 7000;
        return historyTable[m.getFrom()][m.getTo()];
    }

    static void sortMoves(Chess::MoveList& moves, const Chess::Board& board, int ply) {
        int n = static_cast<int>(moves.size());
        int scores[256];
        for (int i = 0; i < n; ++i)
            scores[i] = scoreMoveForOrdering(moves[i], board, ply);

        for (int i = 1; i < n; ++i) {
            Chess::Move mv = moves[i];
            int sc = scores[i];
            int j = i - 1;
            while (j >= 0 && scores[j] < sc) {
                moves[j + 1] = moves[j];
                scores[j + 1] = scores[j];
                --j;
            }
            moves[j + 1] = mv;
            scores[j + 1] = sc;
        }
    }

    // Pawn-structure helpers
    static const Chess::Bitboard FILE_BB[8] = {
        0x0101010101010101ULL, 0x0202020202020202ULL,
        0x0404040404040404ULL, 0x0808080808080808ULL,
        0x1010101010101010ULL, 0x2020202020202020ULL,
        0x4040404040404040ULL, 0x8080808080808080ULL
    };

    static inline Chess::Bitboard ranksAbove(int rank) {
        return (rank < 7) ? (0xFFFFFFFFFFFFFFFFULL << ((rank + 1) * 8)) : 0ULL;
    }
    static inline Chess::Bitboard ranksBelow(int rank) {
        return (rank > 0) ? ((1ULL << (rank * 8)) - 1ULL) : 0ULL;
    }

    static const int PASSED_MG[8] = {  0,  5, 10,  20,  35,  60, 100, 0 };
    static const int PASSED_EG[8] = {  0, 10, 20,  40,  70, 120, 200, 0 };

    // Evaluation -- returns score from side-to-move perspective
    int evaluate(const Chess::Board& board) {
        int mgScore = 0, egScore = 0, phase = 0;
        Chess::Bitboard wPawns = 0, bPawns = 0;
        int wBishops = 0, bBishops = 0;

        for (int sq = 0; sq < 64; ++sq) {
            Chess::PieceType pt = board.pieceAt(static_cast<Chess::Square>(sq));
            if (pt == Chess::PieceType::None) continue;

            int pti = static_cast<int>(pt);
            bool isWhite = (board.colorAt(static_cast<Chess::Square>(sq)) == Chess::ChessColor::White);
            int sign = isWhite ? 1 : -1;
            int pi = pstIndex(sq, isWhite);

            phase += PHASE_INC[pti];
            mgScore += sign * (MG_VALUE[pti] + getPST_MG(pti, pi));
            egScore += sign * (EG_VALUE[pti] + getPST_EG(pti, pi));

            if (pt == Chess::PieceType::Pawn) {
                if (isWhite) wPawns |= (1ULL << sq);
                else bPawns |= (1ULL << sq);
            } else if (pt == Chess::PieceType::Bishop) {
                if (isWhite) wBishops++;
                else bBishops++;
            }
        }

        // Bishop pair bonus
        if (wBishops >= 2) { mgScore += 30; egScore += 50; }
        if (bBishops >= 2) { mgScore -= 30; egScore -= 50; }

        // White pawn structure
        {
            Chess::Bitboard tmp = wPawns;
            while (tmp) {
                int sq   = static_cast<int>(Chess::BitUtils::popLSB(tmp));
                int file = sq & 7;
                int rank = sq >> 3;

                Chess::Bitboard adjFiles = 0;
                if (file > 0) adjFiles |= FILE_BB[file - 1];
                if (file < 7) adjFiles |= FILE_BB[file + 1];
                if (!(adjFiles & wPawns)) { mgScore -= 10; egScore -= 20; }

                Chess::Bitboard span = ((file > 0 ? FILE_BB[file - 1] : 0ULL) |  FILE_BB[file] | (file < 7 ? FILE_BB[file + 1] : 0ULL)) & ranksAbove(rank);
                if (!(span & bPawns)) {
                    mgScore += PASSED_MG[rank];
                    egScore += PASSED_EG[rank];
                }
            }
            for (int f = 0; f < 8; ++f) {
                int cnt = Chess::BitUtils::countBits(wPawns & FILE_BB[f]);
                if (cnt > 1) { mgScore -= 10 * (cnt - 1); egScore -= 20 * (cnt - 1); }
            }
        }

        // Black pawn structure
        {
            Chess::Bitboard tmp = bPawns;
            while (tmp) {
                int sq   = static_cast<int>(Chess::BitUtils::popLSB(tmp));
                int file = sq & 7;
                int rank = sq >> 3;

                Chess::Bitboard adjFiles = 0;
                if (file > 0) adjFiles |= FILE_BB[file - 1];
                if (file < 7) adjFiles |= FILE_BB[file + 1];
                if (!(adjFiles & bPawns)) { mgScore += 10; egScore += 20; }

                Chess::Bitboard span = ((file > 0 ? FILE_BB[file - 1] : 0ULL) |  FILE_BB[file] | (file < 7 ? FILE_BB[file + 1] : 0ULL)) & ranksBelow(rank);
                if (!(span & wPawns)) {
                    int bRank = 7 - rank;
                    mgScore -= PASSED_MG[bRank];
                    egScore -= PASSED_EG[bRank];
                }
            }
            for (int f = 0; f < 8; ++f) {
                int cnt = Chess::BitUtils::countBits(bPawns & FILE_BB[f]);
                if (cnt > 1) { mgScore += 10 * (cnt - 1); egScore += 20 * (cnt - 1); }
            }
        }

        if (phase > MAX_PHASE) phase = MAX_PHASE;
        int score = (mgScore * phase + egScore * (MAX_PHASE - phase)) / MAX_PHASE;

        return (board.getSideToMove() == Chess::ChessColor::White) ? score : -score;
    }

    // Quiescence search
    int quiesce(Chess::Board& board, int alpha, int beta) {
        ++nodes;
        checkTime();
        if (timeOut) return 0;

        int standPat = evaluate(board);
        if (standPat >= beta) return beta;
        if (standPat > alpha) alpha = standPat;

        Chess::MoveList moves;
        board.generatePseudoLegalMoves(moves);

        int n = static_cast<int>(moves.size());
        int scores[256];
        for (int i = 0; i < n; ++i) {
            if (!moves[i].isCapture() && !moves[i].isPromotion()) {
                scores[i] = -1;
                continue;
            }
            Chess::PieceType att = board.pieceAt(moves[i].getFrom());
            Chess::PieceType vic = (moves[i].getFlags() == 5) ? Chess::PieceType::Pawn : board.pieceAt(moves[i].getTo());
            if (att != Chess::PieceType::None && vic != Chess::PieceType::None) {
                int vi = static_cast<int>(vic);
                int ai = static_cast<int>(att);
                scores[i] = MVV_LVA[vi * 6 + ai];
            } else {
                scores[i] = 0;
            }
        }

        for (int i = 0; i < n; ++i) {
            int best = i;
            for (int j = i + 1; j < n; ++j)
                if (scores[j] > scores[best]) best = j;
            if (scores[best] < 0) break;

            Chess::Move tmp  = moves[i];   
            moves[i]   = moves[best];  
            moves[best]  = tmp;
            int ts = scores[i];  
            scores[i]  = scores[best]; 
            scores[best] = ts;

            if (!board.makeMove(moves[i])) continue;
            int score = -quiesce(board, -beta, -alpha);
            board.unmakeMove(moves[i]);

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }

        return alpha;
    }


    // Alpha-Beta with killer moves
    int alphaBeta(Chess::Board& board, int alpha, int beta, int depth, int ply) {
        ++nodes;
        checkTime();
        if (timeOut) return 0;

        if (depth == 0) return quiesce(board, alpha, beta);

        Chess::MoveList moves;
        board.generatePseudoLegalMoves(moves);
        sortMoves(moves, board, ply);

        int legalMoves = 0;

        for (int i = 0, n = static_cast<int>(moves.size()); i < n; ++i) {
            if (!board.makeMove(moves[i])) continue;
            ++legalMoves;

            int score = -alphaBeta(board, -beta, -alpha, depth - 1, ply + 1);
            board.unmakeMove(moves[i]);

            if (timeOut) return 0;

            if (score >= beta) {
                if (!moves[i].isCapture() && !moves[i].isPromotion()) {
                    killerMoves[1][ply] = killerMoves[0][ply];
                    killerMoves[0][ply] = moves[i];
                    historyTable[moves[i].getFrom()][moves[i].getTo()] += depth * depth;
                }
                return beta;
            }
            if (score > alpha) alpha = score;
        }

        if (legalMoves == 0)
            return board.isCheck() ? -(MATE_SCORE - ply) : DRAW_SCORE;

        return alpha;
    }


    // Iterative deepening
    Chess::Move searchBestMove(Chess::Board& board) {
        startTime     = millis();
        lastYieldTime = startTime;
        timeOut       = false;
        forceAbort    = false;
        nodes         = 0;

        memset(killerMoves,  0, sizeof(killerMoves));
        memset(historyTable, 0, sizeof(historyTable));

        Chess::MoveList moves;
        board.generateLegalMoves(moves);

        if (moves.size() == 0) return Chess::Move();
        if (moves.size() == 1) return moves[0];

        Chess::Move bestMove        = moves[0];
        Chess::Move currentBestMove = moves[0];

        for (int depth = 1; depth <= 64; ++depth) {
            int bestScore = -(MATE_SCORE * 2);
            int alpha = -(MATE_SCORE * 2);
            int beta = MATE_SCORE * 2;

            sortMoves(moves, board, 0);

            for (int i = 0, n = static_cast<int>(moves.size()); i < n; ++i) {
                board.makeMove(moves[i]);
                int score = -alphaBeta(board, -beta, -alpha, depth - 1, 1);
                board.unmakeMove(moves[i]);

                if (timeOut) break;

                if (score > bestScore) {
                    bestScore = score;
                    currentBestMove = moves[i];
                }
                if (score > alpha) alpha = score;
            }

            if (timeOut) break;

            bestMove = currentBestMove;
            Serial.printf("Info depth %d score %d nodes %lu\n", depth, bestScore, nodes);
        }

        return bestMove;
    }

} // namespace ChessEngine