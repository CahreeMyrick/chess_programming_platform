#pragma once

#include "core/types.hpp"

#include "position/position.hpp"
#include "position/bb_position.hpp"

namespace chess {

class Eval {
public:
    virtual ~Eval() = default;
    static int evaluate(const Position& pos);
    static int evaluate(const BitboardPosition& pos);  // popcount-based, no square scan
};

};

// namespace chess