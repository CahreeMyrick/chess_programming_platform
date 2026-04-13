#pragma once
#include "position/bb_position.hpp"
#include "search/search_bb.hpp"
#include "openings/opening_book.h"
#include <string>
#include <vector>

namespace chess {

struct PositionSample {
    std::string fen;
    std::string move_uci;
    int         score;
    float       outcome;
};

std::vector<PositionSample> play_one_game(int depth, const PositionBook& book);
void run_selfplay(int num_games, int depth, const std::string& output_path);
} // namespace chess