#pragma once
#include "Chess/chess.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace ChessEngine {

    // Configuration
    void setTimeLimit(uint32_t milliseconds);
    void abortSearch();

    // Runs the iterative deepening search and returns the best move found
    Chess::Move searchBestMove(Chess::Board& board);

    // Internal evaluation
    int evaluate(const Chess::Board& board);
}