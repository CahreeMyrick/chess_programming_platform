#pragma once

#include <array>
#include <memory>
#include "position/board.hpp"
#include "position/piece.hpp"

namespace chess::pointerboard {

class PointerBoard : public chess::Board {
public:
    PointerBoard() = default;

    chess::Piece piece_at(int sq) const override;
    void set_piece(int sq, chess::Piece p) override;
    void clear_square(int sq) override;

private:
    std::array<std::unique_ptr<PieceObject>, 64> board_{};
};

} // namespace chess::pointerboard