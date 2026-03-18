#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "core/types.hpp"
#include "core/move.hpp"
#include "position/board.hpp"

namespace chess {

class Position {
public:
    explicit Position(std::unique_ptr<Board> board);

    Position(const Position& other);
    Position& operator=(const Position& other);

    Position(Position&&) noexcept = default;
    Position& operator=(Position&&) noexcept = default;

    static Position startpos(std::unique_ptr<Board> board);

    Piece at(int sq) const;
    void set(int sq, Piece p);
    void clear(int sq);
    void move_piece(int from, int to);

    Color side_to_move() const;
    void set_side_to_move(Color c);

    uint8_t castling_rights() const;
    void set_castling_rights(uint8_t cr);

    int ep_square() const;
    void set_ep_square(int sq);

    bool make_move(const Move& m, std::string& err);

private:
    std::unique_ptr<Board> board_;
    Color stm_ = Color::White;
    uint8_t cr_ = CR_NONE;
    int ep_ = -1;
};

} // namespace chess