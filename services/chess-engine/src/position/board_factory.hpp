#pragma once

#include <memory>
#include "position/board.hpp"

namespace chess {

enum class BoardType {
    Array,
    Pointer,
    Bitboard
};

std::unique_ptr<Board> make_board(BoardType type);

} // namespace chess