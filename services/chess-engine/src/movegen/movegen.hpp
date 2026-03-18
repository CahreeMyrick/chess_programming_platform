#pragma once

#include "core/types.hpp"
#include "core/move.hpp"
#include "core/movelist.hpp"
#include "position/position.hpp"

namespace chess {

class MoveGen {
public:
    static bool is_friend_piece(Piece a, Piece b);
    static bool is_opponent_piece(Piece a, Piece b);

    static void gen_pawn(const Position& pos, int from, Piece p, MoveList& out);
    static void gen_knight(const Position& pos, int from, Piece p, MoveList& out);
    static void gen_bishop(const Position& pos, int from, Piece p, MoveList& out);
    static void gen_rook(const Position& pos, int from, Piece p, MoveList& out);
    static void gen_queen(const Position& pos, int from, Piece p, MoveList& out);
    static void gen_king(const Position& pos, int from, Piece p, MoveList& out);

    static void gen_ray(const Position& pos, int from, Piece p, int df_step, int dr_step, MoveList& out);

    static void generate_pseudo_legal(const Position& pos, MoveList& out);
    static void generate_legal(const Position& pos, MoveList& out);

    static bool has_any_legal_move(const Position& pos);
};

} // namespace chess