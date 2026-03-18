#pragma once

#include <cstdint>
#include "position/board.hpp"

namespace chess {

class Bitboard : public Board {
public:
    Bitboard();

    Piece piece_at(int sq) const override;
    void set_piece(int sq, Piece p) override;
    void clear_square(int sq) override;

private:
    uint64_t wp_ = 0ULL;
    uint64_t wn_ = 0ULL;
    uint64_t wb_ = 0ULL;
    uint64_t wr_ = 0ULL;
    uint64_t wq_ = 0ULL;
    uint64_t wk_ = 0ULL;

    uint64_t bp_ = 0ULL;
    uint64_t bn_ = 0ULL;
    uint64_t bb_ = 0ULL;
    uint64_t br_ = 0ULL;
    uint64_t bq_ = 0ULL;
    uint64_t bk_ = 0ULL;

    static uint64_t bit(int sq);
    void clear_all_at(int sq);
};

} // namespace chess