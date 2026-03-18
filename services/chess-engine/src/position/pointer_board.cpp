#include "position/pointer_board.hpp"

namespace chess::pointerboard {

chess::Piece PointerBoard::piece_at(int sq) const {
    if (!board_[sq]) {
        return chess::Piece::Empty;
    }
    return board_[sq]->code();
}

void PointerBoard::set_piece(int sq, chess::Piece p) {
    clear_square(sq);

    if (p == chess::Piece::Empty) {
        return;
    }

    board_[sq] = make_piece_object(p);
}

void PointerBoard::clear_square(int sq) {
    board_[sq].reset();
}

} // namespace chess::pointerboard