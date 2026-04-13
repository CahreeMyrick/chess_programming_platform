#include "position/position.hpp"
#include "rules/rules.hpp"

#include <cstdlib>
#include <utility>
#include <sstream>
#include <algorithm>

namespace chess {

// ── Zobrist tables ────────────────────────────────────────────────────────────
// Generated once at startup with a fixed seed — reproducible across runs.

namespace {

struct ZobristTable {
    uint64_t pieces[64][13];   // [square][piece]  (index 0 = Empty, unused)
    uint64_t side_to_move;     // XOR in when Black to move
    uint64_t castling[16];     // [castling rights bitmask]
    uint64_t en_passant[8];    // [file 0-7]

    ZobristTable() {
        uint64_t s = 6364136223846793005ULL; // fixed seed
        auto next = [&]() {
            s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
            return s * 2685821657736338717ULL;
        };

        for (auto& sq : pieces)
            for (auto& p : sq)
                p = next();

        side_to_move = next();

        for (auto& c : castling)   c = next();
        for (auto& e : en_passant) e = next();
    }
};

const ZobristTable ZOBRIST;

// Map Piece enum to a table index (0 reserved for Empty)
int piece_index(Piece p) {
    switch (p) {
        case Piece::WK: return 1;  case Piece::WQ: return 2;
        case Piece::WN: return 3;  case Piece::WB: return 4;
        case Piece::WP: return 5;  case Piece::WR: return 6;
        case Piece::BK: return 7;  case Piece::BQ: return 8;
        case Piece::BN: return 9;  case Piece::BB: return 10;
        case Piece::BP: return 11; case Piece::BR: return 12;
        default:        return 0;
    }
}

} // namespace

// ── Zobrist key ───────────────────────────────────────────────────────────────

uint64_t Position::zobrist_key() const {
    uint64_t key = 0;

    for (int sq = 0; sq < 64; sq++) {
        Piece p = at(sq);
        if (p != Piece::Empty)
            key ^= ZOBRIST.pieces[sq][piece_index(p)];
    }

    if (stm_ == Color::Black)
        key ^= ZOBRIST.side_to_move;

    key ^= ZOBRIST.castling[cr_];

    if (ep_ != -1)
        key ^= ZOBRIST.en_passant[file_of(ep_)];

    return key;
}

// ── Draw detection ────────────────────────────────────────────────────────────

int Position::halfmove_clock() const { return halfmove_; }
void Position::set_halfmove_clock(int n) { halfmove_ = n; }

bool Position::is_draw_by_fifty() const {
    return halfmove_ >= 100; // 100 half-moves = 50 full moves
}

bool Position::is_draw_by_repetition() const {
    uint64_t key = zobrist_key();
    int count = 0;
    for (uint64_t k : history_)
        if (k == key) count++;
    return count >= 2; // current position + 2 in history = threefold
}

int Position::winner() const { return winner_; }
void Position::set_winner(int winner) { winner_ = winner; }

// ── Constructors ──────────────────────────────────────────────────────────────

Position::Position(std::unique_ptr<Board> board)
    : board_(std::move(board)) {}

Position::Position(const Position& other)
    : board_(other.board_ ? other.board_->clone() : nullptr),
      stm_(other.stm_),
      cr_(other.cr_),
      ep_(other.ep_),
      halfmove_(other.halfmove_),
      history_(other.history_) {}

Position& Position::operator=(const Position& other) {
    if (this == &other) return *this;

    board_    = other.board_ ? other.board_->clone() : nullptr;
    stm_      = other.stm_;
    cr_       = other.cr_;
    ep_       = other.ep_;
    halfmove_ = other.halfmove_;
    history_  = other.history_;

    return *this;
}

// ── Static factories ──────────────────────────────────────────────────────────

Position Position::startpos(std::unique_ptr<Board> board) {
    Position p(std::move(board));

    for (const auto& entry : STARTPOS_PLACEMENTS)
        p.set(entry.sq, entry.pc);

    p.set_side_to_move(Color::White);
    p.set_ep_square(-1);
    p.set_castling_rights(CR_WK | CR_WQ | CR_BK | CR_BQ);

    return p;
}

Position Position::from_fen(const std::string& fen, std::unique_ptr<Board> board) {
    Position p(std::move(board));

    std::istringstream ss(fen);
    std::string placement, side, castling, enpassant;
    ss >> placement >> side >> castling >> enpassant;

    for (int sq = 0; sq < 64; sq++) p.clear(sq);

    int sq = 56;
    for (char c : placement) {
        if (c == '/') {
            sq -= 16;
        } else if (c >= '1' && c <= '8') {
            sq += (c - '0');
        } else {
            Piece pc = Piece::Empty;
            switch (c) {
                case 'K': pc = Piece::WK; break; case 'Q': pc = Piece::WQ; break;
                case 'N': pc = Piece::WN; break; case 'B': pc = Piece::WB; break;
                case 'P': pc = Piece::WP; break; case 'R': pc = Piece::WR; break;
                case 'k': pc = Piece::BK; break; case 'q': pc = Piece::BQ; break;
                case 'n': pc = Piece::BN; break; case 'b': pc = Piece::BB; break;
                case 'p': pc = Piece::BP; break; case 'r': pc = Piece::BR; break;
            }
            if (pc != Piece::Empty) p.set(sq, pc);
            sq++;
        }
    }

    p.set_side_to_move(side == "b" ? Color::Black : Color::White);

    uint8_t cr = CR_NONE;
    for (char c : castling) {
        switch (c) {
            case 'K': cr |= CR_WK; break; case 'Q': cr |= CR_WQ; break;
            case 'k': cr |= CR_BK; break; case 'q': cr |= CR_BQ; break;
        }
    }
    p.set_castling_rights(cr);

    if (enpassant != "-") {
        int file = enpassant[0] - 'a';
        int rank = enpassant[1] - '1';
        p.set_ep_square(make_sq(file, rank));
    } else {
        p.set_ep_square(-1);
    }

    return p;
}

// ── Board accessors ───────────────────────────────────────────────────────────

Piece Position::at(int sq)             const { return board_->piece_at(sq); }
void  Position::set(int sq, Piece p)         { board_->set_piece(sq, p); }
void  Position::clear(int sq)                { board_->clear_square(sq); }
void  Position::move_piece(int from, int to) { board_->move_piece(from, to); }

Color   Position::side_to_move()             const { return stm_; }
void    Position::set_side_to_move(Color c)        { stm_ = c; }
uint8_t Position::castling_rights()          const { return cr_; }
void    Position::set_castling_rights(uint8_t cr)  { cr_ = cr; }
int     Position::ep_square()                const { return ep_; }
void    Position::set_ep_square(int sq)            { ep_ = sq; }

// ── make_move ─────────────────────────────────────────────────────────────────

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

    if (!Rules::is_pseudo_legal(*this, m, err))
        return false;

    Piece moving   = at(m.from);
    Piece captured = at(m.to);

    bool castle  = Rules::is_castle_move(*this, m);
    bool is_pawn = (moving == Piece::WP || moving == Piece::BP);

    bool is_promotion = false;
    if (is_pawn) {
        int last_rank = (moving == Piece::WP) ? 7 : 0;
        is_promotion  = (rank_of(m.to) == last_rank);
    }

    bool is_ep_capture  = false;
    int  ep_captured_sq = -1;

    if (is_pawn && captured == Piece::Empty) {
        int dfv = file_of(m.to) - file_of(m.from);
        int drv = rank_of(m.to) - rank_of(m.from);
        int dir = (moving == Piece::WP) ? +1 : -1;

        if (std::abs(dfv) == 1 && drv == dir && ep_ == m.to) {
            is_ep_capture  = true;
            ep_captured_sq = make_sq(file_of(m.to), rank_of(m.to) - dir);
        }
    }

    if (castle && !Rules::castle_path_safe(*this, m, err))
        return false;

    // Legality check on a temporary copy
    Position tmp = *this;
    tmp.set(m.to, moving);
    tmp.set(m.from, Piece::Empty);
    if (is_ep_capture) tmp.set(ep_captured_sq, Piece::Empty);
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
    if (is_promotion) tmp.set(m.to, promoted_piece(moving, m.promo));

    if (Rules::in_check(tmp, side_to_move())) {
        err = "Move is illegal: it leaves your king in check.";
        return false;
    }

    // ── Commit the move ───────────────────────────────────────────────────────

    // Record position BEFORE the move for repetition tracking
    history_.push_back(zobrist_key());

    set(m.to, moving);
    set(m.from, Piece::Empty);
    if (is_ep_capture) set(ep_captured_sq, Piece::Empty);
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
    if (is_promotion) set(m.to, promoted_piece(moving, m.promo));

    // ── Castling rights ───────────────────────────────────────────────────────
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

    // ── Half-move clock (fifty move rule) ─────────────────────────────────────
    bool is_capture = (captured != Piece::Empty) || is_ep_capture;
    if (is_pawn || is_capture)
        halfmove_ = 0;
    else
        halfmove_++;

    // ── En passant square ─────────────────────────────────────────────────────
    ep_ = -1;
    if (is_pawn) {
        int drv = rank_of(m.to) - rank_of(m.from);
        int dir = (moving == Piece::WP) ? +1 : -1;
        if (drv == 2 * dir)
            ep_ = make_sq(file_of(m.from), rank_of(m.from) + dir);
    }

    stm_ = other(stm_);
    return true;
}

} // namespace chess
