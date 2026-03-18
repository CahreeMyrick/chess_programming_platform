#include "movegen/movegen.hpp"
#include "rules/rules.hpp"
#include <cstdlib>
#include <string>

namespace chess {

static bool side_owns_piece(Color side, Piece p) {
    return (side == Color::White) ? is_white(p) : is_black(p);
}

bool MoveGen::is_friend_piece(Piece a, Piece b) {
    if (is_empty(b)) return false;
    return (is_white(a) && is_white(b)) || (is_black(a) && is_black(b));
}

bool MoveGen::is_opponent_piece(Piece a, Piece b) {
    if (is_empty(b)) return false;
    return (is_white(a) && is_black(b)) || (is_black(a) && is_white(b));
}

void MoveGen::gen_pawn(const Position& pos, int from, Piece p, MoveList& out) {
    const bool white = is_white(p);
    const int dir = white ? +1 : -1;
    const int start_rank = white ? 1 : 6;
    const int last_rank = white ? 7 : 0;

    const int f = file_of(from);
    const int r = rank_of(from);

    auto push_pawn_move = [&](int to) {
        const bool promotes = (rank_of(to) == last_rank);

        if (!promotes) {
            out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(to), PROMO_NONE});
        } else {
            out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(to), PROMO_Q});
            out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(to), PROMO_R});
            out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(to), PROMO_B});
            out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(to), PROMO_N});
        }
    };

    int r1 = r + dir;
    if (r1 >= 0 && r1 < 8) {
        int to = make_sq(f, r1);
        if (is_empty(pos.at(to))) {
            push_pawn_move(to);

            if (r == start_rank) {
                int r2 = r + 2 * dir;
                int to2 = make_sq(f, r2);
                if (is_empty(pos.at(to2))) {
                    out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(to2), PROMO_NONE});
                }
            }
        }
    }

    int cap_r = r + dir;
    if (cap_r >= 0 && cap_r < 8) {
        if (f - 1 >= 0) {
            int to = make_sq(f - 1, cap_r);
            if (is_opponent_piece(p, pos.at(to))) {
                push_pawn_move(to);
            }
        }

        if (f + 1 < 8) {
            int to = make_sq(f + 1, cap_r);
            if (is_opponent_piece(p, pos.at(to))) {
                push_pawn_move(to);
            }
        }
    }

    int ep = pos.ep_square();
    if (ep != -1) {
        int ep_f = file_of(ep);
        int ep_r = rank_of(ep);

        if (ep_r == r + dir && std::abs(ep_f - f) == 1) {
            int cap_sq = make_sq(ep_f, ep_r - dir);
            Piece cap = pos.at(cap_sq);

            if (white && cap == Piece::BP) {
                out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(ep), PROMO_NONE});
            } else if (!white && cap == Piece::WP) {
                out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(ep), PROMO_NONE});
            }
        }
    }
}

void MoveGen::gen_knight(const Position& pos, int from, Piece p, MoveList& out) {
    static const int d[8][2] = {
        {+1,+2},{+2,+1},{+2,-1},{+1,-2},
        {-1,-2},{-2,-1},{-2,+1},{-1,+2}
    };

    int f0 = file_of(from);
    int r0 = rank_of(from);

    for (const auto& dd : d) {
        int f = f0 + dd[0];
        int r = r0 + dd[1];

        if (f < 0 || f >= 8 || r < 0 || r >= 8) continue;

        int to = make_sq(f, r);
        Piece t = pos.at(to);

        if (is_friend_piece(p, t)) continue;

        out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(to), PROMO_NONE});
    }
}

void MoveGen::gen_ray(const Position& pos, int from, Piece p, int df_step, int dr_step, MoveList& out) {
    int f = file_of(from) + df_step;
    int r = rank_of(from) + dr_step;

    while (f >= 0 && f < 8 && r >= 0 && r < 8) {
        int to = make_sq(f, r);
        Piece t = pos.at(to);

        if (is_empty(t)) {
            out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(to), PROMO_NONE});
        } else {
            if (is_opponent_piece(p, t)) {
                out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(to), PROMO_NONE});
            }
            break;
        }

        f += df_step;
        r += dr_step;
    }
}

void MoveGen::gen_bishop(const Position& pos, int from, Piece p, MoveList& out) {
    gen_ray(pos, from, p, +1, +1, out);
    gen_ray(pos, from, p, -1, +1, out);
    gen_ray(pos, from, p, +1, -1, out);
    gen_ray(pos, from, p, -1, -1, out);
}

void MoveGen::gen_rook(const Position& pos, int from, Piece p, MoveList& out) {
    gen_ray(pos, from, p, +1,  0, out);
    gen_ray(pos, from, p, -1,  0, out);
    gen_ray(pos, from, p,  0, +1, out);
    gen_ray(pos, from, p,  0, -1, out);
}

void MoveGen::gen_queen(const Position& pos, int from, Piece p, MoveList& out) {
    gen_bishop(pos, from, p, out);
    gen_rook(pos, from, p, out);
}

void MoveGen::gen_king(const Position& pos, int from, Piece p, MoveList& out) {
    int f0 = file_of(from);
    int r0 = rank_of(from);

    for (int df = -1; df <= 1; ++df) {
        for (int dr = -1; dr <= 1; ++dr) {
            if (df == 0 && dr == 0) continue;

            int f = f0 + df;
            int r = r0 + dr;

            if (f < 0 || f >= 8 || r < 0 || r >= 8) continue;

            int to = make_sq(f, r);
            Piece t = pos.at(to);

            if (is_friend_piece(p, t)) continue;

            out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(to), PROMO_NONE});
        }
    }

    bool white = is_white(p);
    int r = white ? 0 : 7;

    if (file_of(from) == 4 && rank_of(from) == r) {
        uint8_t cr = pos.castling_rights();

        if (white) {
            if ((cr & CR_WK) &&
                pos.at(make_sq(5,0)) == Piece::Empty &&
                pos.at(make_sq(6,0)) == Piece::Empty &&
                pos.at(make_sq(7,0)) == Piece::WR) {
                out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(make_sq(6,0)), PROMO_NONE});
            }

            if ((cr & CR_WQ) &&
                pos.at(make_sq(3,0)) == Piece::Empty &&
                pos.at(make_sq(2,0)) == Piece::Empty &&
                pos.at(make_sq(1,0)) == Piece::Empty &&
                pos.at(make_sq(0,0)) == Piece::WR) {
                out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(make_sq(2,0)), PROMO_NONE});
            }
        } else {
            if ((cr & CR_BK) &&
                pos.at(make_sq(5,7)) == Piece::Empty &&
                pos.at(make_sq(6,7)) == Piece::Empty &&
                pos.at(make_sq(7,7)) == Piece::BR) {
                out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(make_sq(6,7)), PROMO_NONE});
            }

            if ((cr & CR_BQ) &&
                pos.at(make_sq(3,7)) == Piece::Empty &&
                pos.at(make_sq(2,7)) == Piece::Empty &&
                pos.at(make_sq(1,7)) == Piece::Empty &&
                pos.at(make_sq(0,7)) == Piece::BR) {
                out.push(Move{static_cast<uint8_t>(from), static_cast<uint8_t>(make_sq(2,7)), PROMO_NONE});
            }
        }
    }
}

void MoveGen::generate_pseudo_legal(const Position& pos, MoveList& out) {
    out.clear();
    Color stm = pos.side_to_move();

    for (int from = 0; from < 64; ++from) {
        Piece p = pos.at(from);

        if (is_empty(p)) continue;
        if (!side_owns_piece(stm, p)) continue;

        switch (p) {
            case Piece::WP:
            case Piece::BP:
                gen_pawn(pos, from, p, out);
                break;

            case Piece::WN:
            case Piece::BN:
                gen_knight(pos, from, p, out);
                break;

            case Piece::WB:
            case Piece::BB:
                gen_bishop(pos, from, p, out);
                break;

            case Piece::WR:
            case Piece::BR:
                gen_rook(pos, from, p, out);
                break;

            case Piece::WQ:
            case Piece::BQ:
                gen_queen(pos, from, p, out);
                break;

            case Piece::WK:
            case Piece::BK:
                gen_king(pos, from, p, out);
                break;

            default:
                break;
        }
    }
}

void MoveGen::generate_legal(const Position& pos, MoveList& out) {
    MoveList pseudo;
    generate_pseudo_legal(pos, pseudo);

    out.clear();
    Color mover = pos.side_to_move();

    for (int i = 0; i < pseudo.size; ++i) {
        const Move& m = pseudo.moves[i];

        if (Rules::is_castle_move(pos, m)) {
            std::string cerr;
            if (!Rules::castle_path_safe(pos, m, cerr)) continue;
        }

        Position tmp = pos;
        Piece moving = tmp.at(m.from);

        tmp.set(m.to, moving);
        tmp.set(m.from, Piece::Empty);

        if (Rules::is_castle_move(pos, m)) {
            int r = (moving == Piece::WK) ? 0 : 7;
            bool king_side = (file_of(m.to) == 6);

            if (king_side) {
                tmp.set(make_sq(5, r), tmp.at(make_sq(7, r)));
                tmp.set(make_sq(7, r), Piece::Empty);
            } else {
                tmp.set(make_sq(3, r), tmp.at(make_sq(0, r)));
                tmp.set(make_sq(0, r), Piece::Empty);
            }
        }

        if ((moving == Piece::WP || moving == Piece::BP) &&
            pos.ep_square() == m.to &&
            is_empty(pos.at(m.to))) {

            int dfv = file_of(m.to) - file_of(m.from);
            int drv = rank_of(m.to) - rank_of(m.from);
            int dir = (moving == Piece::WP) ? +1 : -1;

            if (std::abs(dfv) == 1 && drv == dir) {
                int cap_sq = make_sq(file_of(m.to), rank_of(m.to) - dir);
                tmp.set(cap_sq, Piece::Empty);
            }
        }

        if (moving == Piece::WP || moving == Piece::BP) {
            int last_rank = (moving == Piece::WP) ? 7 : 0;
            if (rank_of(m.to) == last_rank) {
                uint8_t pr = m.promo;
                if (pr == PROMO_NONE) pr = PROMO_Q;

                Piece promoted = Piece::Empty;
                bool white = (moving == Piece::WP);

                switch (pr) {
                    case PROMO_Q: promoted = white ? Piece::WQ : Piece::BQ; break;
                    case PROMO_R: promoted = white ? Piece::WR : Piece::BR; break;
                    case PROMO_B: promoted = white ? Piece::WB : Piece::BB; break;
                    case PROMO_N: promoted = white ? Piece::WN : Piece::BN; break;
                    default:      promoted = white ? Piece::WQ : Piece::BQ; break;
                }

                tmp.set(m.to, promoted);
            }
        }

        if (Rules::in_check(tmp, mover)) continue;

        out.push(m);
    }
}

bool MoveGen::has_any_legal_move(const Position& pos) {
    MoveList legal;
    generate_legal(pos, legal);
    return legal.size > 0;
}

} // namespace chess