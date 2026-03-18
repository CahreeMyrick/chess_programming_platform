#include "position/position.hpp"
#include <utility>

namespace chess {

Position::Position(std::unique_ptr<Board> board)
    : board_(std::move(board)) {}

Position Position::startpos(std::unique_ptr<Board> board) {
    Position p(std::move(board));

    for (const auto& entry : STARTPOS_PLACEMENTS) {
        p.set(entry.sq, entry.pc);
    }

    p.set_side_to_move(Color::White);
    p.set_ep_square(-1);
    p.set_castling_rights(CR_WK | CR_WQ | CR_BK | CR_BQ);

    return p;
}

Piece Position::at(int sq) const {
    return board_->piece_at(sq);
}

void Position::set(int sq, Piece p) {
    board_->set_piece(sq, p);
}

void Position::clear(int sq) {
    board_->clear_square(sq);
}

void Position::move_piece(int from, int to) {
    board_->move_piece(from, to);
}

Color Position::side_to_move() const {
    return stm_;
}

void Position::set_side_to_move(Color c) {
    stm_ = c;
}

uint8_t Position::castling_rights() const {
    return cr_;
}

void Position::set_castling_rights(uint8_t cr) {
    cr_ = cr;
}

int Position::ep_square() const {
    return ep_;
}

void Position::set_ep_square(int sq) {
    ep_ = sq;
}

} // namespace chess