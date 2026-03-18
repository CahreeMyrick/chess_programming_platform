#pragma once

#include "position/position.hpp"

namespace chess {

class Eval {
public:
    static int evaluate(const Position& pos);
};

} // namespace chess