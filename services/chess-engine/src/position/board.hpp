#pragma once

#include "core/types.hpp"

namespace chess {

class Board {
public:
    virtual ~Board() = default;

    virtual Piece piece_at(int sq) const = 0;
    virtual void set_piece(int sq, Piece p) = 0;
    virtual void clear_square(int sq) = 0;

    virtual void move_piece(int from, int to) {
        Piece p = piece_at(from);
        clear_square(from);
        set_piece(to, p);
    }
};

} // namespace chess