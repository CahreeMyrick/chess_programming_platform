#include "position/board_factory.hpp"

#include "position/array_board.hpp"
#include "position/pointer_board.hpp"
#include "position/bitboard.hpp"

namespace chess {

std::unique_ptr<Board> make_board(BoardType type) {
    switch (type) {
        case BoardType::Array:
            return std::make_unique<ArrayBoard>();

        case BoardType::Pointer:
            return std::make_unique<pointerboard::PointerBoard>();

        case BoardType::Bitboard:
            return std::make_unique<Bitboard>();
    }

    return nullptr;
}

} // namespace chess