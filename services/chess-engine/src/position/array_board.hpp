#pragma once

#include <array>
#include "position/board.hpp"

namespace chess {

class ArrayBoard : public Board {
public:
    ArrayBoard();

    Piece piece_at(int sq) const override;
    void set_piece(int sq, Piece p) override;
    void clear_square(int sq) override;
    std::unique_ptr<Board> clone() const override;
    
private:
    std::array<Piece, 64> board_;
};

} // namespace chess