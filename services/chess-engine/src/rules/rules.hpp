#pragma once

#include <string>
#include "core/types.hpp"
#include "core/move.hpp"
#include "position/position.hpp"

namespace chess {

class Rules {
public:
    static bool is_friend(Piece moving, Piece target);
    static bool is_opponent(Piece moving, Piece target);

    static bool path_clear(const Position& pos, int from, int to);

    static bool pseudo_pawn(const Position& pos, Piece p, const Move& m, std::string& err);
    static bool pseudo_knight(const Position& pos, Piece p, int from, int to, std::string& err);
    static bool pseudo_bishop(const Position& pos, Piece p, int from, int to, std::string& err);
    static bool pseudo_rook(const Position& pos, Piece p, int from, int to, std::string& err);
    static bool pseudo_queen(const Position& pos, Piece p, int from, int to, std::string& err);
    static bool pseudo_king(const Position& pos, Piece p, int from, int to, std::string& err);

    static bool is_pseudo_legal(const Position& pos, const Move& m, std::string& err);

    static int find_king(const Position& pos, Color who);
    static bool is_square_attacked(const Position& pos, int sq, Color by);
    static bool in_check(const Position& pos, Color who);

    static bool is_castle_move(const Position& pos, const Move& m);
    static bool castle_path_safe(const Position& pos, const Move& m, std::string& err);
};

} // namespace chess