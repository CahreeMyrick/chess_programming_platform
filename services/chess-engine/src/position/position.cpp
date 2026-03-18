#include "position/position.hpp"
#include "rules/rules.hpp"

#include <cstdlib>
#include <utility>

namespace chess {

Position::Position(std::unique_ptr<Board> board)
    : board_(std::move(board)) {}

Position::Position(const Position& other)
    : board_(other.board_ ? other.board_->clone() : nullptr),
      stm_(other.stm_),
      cr_(other.cr_),
      ep_(other.ep_) {}

Position& Position::operator=(const Position& other) {
    if (this == &other) {
        return *this;
    }

    board_ = other.board_ ? other.board_->clone() : nullptr;
    stm_ = other.stm_;
    cr_ = other.cr_;
    ep_ = other.ep_;

    return *this;
}

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

static Piece promoted_piece(Piece pawn, uint8_t promo) {
    const bool white = (pawn == Piece::WP);
    uint8_t p = promo;
    if (p == PROMO_NONE) p = PROMO_Q;

    switch (p) {
        case PROMO_Q: return white ? Piece::WQ : Piece::BQ;
        case PROMO_R: return white ? Piece::WR : Piece::BR;
        case PROMO_B: return white ? Piece::WB : Piece::BB;
        case PROMO_N: return white ? Piece::WN : Piece::BN;
        default:      return white ? Piece::WQ : Piece::BQ;
    }
}

bool Position::make_move(const Move& m, std::string& err) {
    err.clear();

    if (!Rules::is_pseudo_legal(*this, m, err)) {
        return false;
    }

    Piece moving = at(m.from);
    Piece captured = at(m.to);

    bool castle = Rules::is_castle_move(*this, m);
    bool is_pawn = (moving == Piece::WP || moving == Piece::BP);

    bool is_promotion = false;
    if (is_pawn) {
        int last_rank = (moving == Piece::WP) ? 7 : 0;
        is_promotion = (rank_of(m.to) == last_rank);
    }

    bool is_ep_capture = false;
    int ep_captured_sq = -1;

    if (is_pawn && captured == Piece::Empty) {
        int dfv = file_of(m.to) - file_of(m.from);
        int drv = rank_of(m.to) - rank_of(m.from);
        int dir = (moving == Piece::WP) ? +1 : -1;

        if (std::abs(dfv) == 1 && drv == dir && ep_ == m.to) {
            is_ep_capture = true;
            ep_captured_sq = make_sq(file_of(m.to), rank_of(m.to) - dir);
        }
    }

    if (castle) {
        if (!Rules::castle_path_safe(*this, m, err)) {
            return false;
        }
    }

    Position tmp = *this;

    tmp.set(m.to, moving);
    tmp.set(m.from, Piece::Empty);

    if (is_ep_capture) {
        tmp.set(ep_captured_sq, Piece::Empty);
    }

    if (castle) {
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

    if (is_promotion) {
        tmp.set(m.to, promoted_piece(moving, m.promo));
    }

    Color mover = side_to_move();
    if (Rules::in_check(tmp, mover)) {
        err = "Move is illegal: it leaves your king in check.";
        return false;
    }

    set(m.to, moving);
    set(m.from, Piece::Empty);

    if (is_ep_capture) {
        set(ep_captured_sq, Piece::Empty);
    }

    if (castle) {
        int r = (moving == Piece::WK) ? 0 : 7;
        bool king_side = (file_of(m.to) == 6);

        if (king_side) {
            set(make_sq(5, r), at(make_sq(7, r)));
            set(make_sq(7, r), Piece::Empty);
        } else {
            set(make_sq(3, r), at(make_sq(0, r)));
            set(make_sq(0, r), Piece::Empty);
        }
    }

    if (is_promotion) {
        set(m.to, promoted_piece(moving, m.promo));
    }

    {
        auto clear_rights = [&](uint8_t flags) {
            cr_ = static_cast<uint8_t>(cr_ & ~flags);
        };

        if (moving == Piece::WK) clear_rights(CR_WK | CR_WQ);
        if (moving == Piece::BK) clear_rights(CR_BK | CR_BQ);

        if (moving == Piece::WR) {
            if (m.from == make_sq(0, 0)) clear_rights(CR_WQ);
            if (m.from == make_sq(7, 0)) clear_rights(CR_WK);
        }

        if (moving == Piece::BR) {
            if (m.from == make_sq(0, 7)) clear_rights(CR_BQ);
            if (m.from == make_sq(7, 7)) clear_rights(CR_BK);
        }

        if (captured == Piece::WR) {
            if (m.to == make_sq(0, 0)) clear_rights(CR_WQ);
            if (m.to == make_sq(7, 0)) clear_rights(CR_WK);
        }

        if (captured == Piece::BR) {
            if (m.to == make_sq(0, 7)) clear_rights(CR_BQ);
            if (m.to == make_sq(7, 7)) clear_rights(CR_BK);
        }
    }

    ep_ = -1;
    if (moving == Piece::WP || moving == Piece::BP) {
        int drv = rank_of(m.to) - rank_of(m.from);
        int dir = (moving == Piece::WP) ? +1 : -1;

        if (drv == 2 * dir) {
            ep_ = make_sq(file_of(m.from), rank_of(m.from) + dir);
        }
    }

    stm_ = other(stm_);
    return true;
}

} // namespace chess