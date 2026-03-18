#pragma once

#include <string>
#include "core/types.hpp"
#include "position/position.hpp"

namespace chess {

class Render {
public:
    static char piece_char(Piece p);
    static std::string board_ascii(const Position& pos);
};

} // namespace chess