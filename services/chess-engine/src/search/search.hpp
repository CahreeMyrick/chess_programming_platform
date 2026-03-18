#pragma once

#include "core/move.hpp"
#include "position/position.hpp"

namespace chess {

struct SearchResult {
    Move best{};
    int score = 0;
};

class Search {
public:
    static SearchResult minimax(Position& pos, int depth);
};

} // namespace chess