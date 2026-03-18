#include "position/bitboard.hpp"

namespace chess {

Bitboard::Bitboard() = default;

uint64_t Bitboard::bit(int sq) {
    return 1ULL << sq;
}

void Bitboard::clear_all_at(int sq) {
    uint64_t mask = ~bit(sq);

    wp_ &= mask;
    wn_ &= mask;
    wb_ &= mask;
    wr_ &= mask;
    wq_ &= mask;
    wk_ &= mask;

    bp_ &= mask;
    bn_ &= mask;
    bb_ &= mask;
    br_ &= mask;
    bq_ &= mask;
    bk_ &= mask;
}

Piece Bitboard::piece_at(int sq) const {
    uint64_t b = bit(sq);

    if (wp_ & b) return Piece::WP;
    if (wn_ & b) return Piece::WN;
    if (wb_ & b) return Piece::WB;
    if (wr_ & b) return Piece::WR;
    if (wq_ & b) return Piece::WQ;
    if (wk_ & b) return Piece::WK;

    if (bp_ & b) return Piece::BP;
    if (bn_ & b) return Piece::BN;
    if (bb_ & b) return Piece::BB;
    if (br_ & b) return Piece::BR;
    if (bq_ & b) return Piece::BQ;
    if (bk_ & b) return Piece::BK;

    return Piece::Empty;
}

void Bitboard::set_piece(int sq, Piece p) {
    clear_all_at(sq);

    uint64_t b = bit(sq);

    switch (p) {
        case Piece::WP: wp_ |= b; break;
        case Piece::WN: wn_ |= b; break;
        case Piece::WB: wb_ |= b; break;
        case Piece::WR: wr_ |= b; break;
        case Piece::WQ: wq_ |= b; break;
        case Piece::WK: wk_ |= b; break;

        case Piece::BP: bp_ |= b; break;
        case Piece::BN: bn_ |= b; break;
        case Piece::BB: bb_ |= b; break;
        case Piece::BR: br_ |= b; break;
        case Piece::BQ: bq_ |= b; break;
        case Piece::BK: bk_ |= b; break;

        case Piece::Empty:
        default:
            break;
    }
}

void Bitboard::clear_square(int sq) {
    clear_all_at(sq);
}

std::unique_ptr<Board> Bitboard::clone() const {
    return std::make_unique<Bitboard>(*this);
}

} // namespace chess