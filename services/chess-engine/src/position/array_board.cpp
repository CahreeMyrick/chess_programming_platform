#include "position/array_board.hpp"

namespace chess {

ArrayBoard::ArrayBoard() {
    board_.fill(Piece::Empty);
}

Piece ArrayBoard::piece_at(int sq) const {
    return board_[sq];
}

void ArrayBoard::set_piece(int sq, Piece p) {
    board_[sq] = p;
}

void ArrayBoard::clear_square(int sq) {
    board_[sq] = Piece::Empty;
}

} // namespace chess