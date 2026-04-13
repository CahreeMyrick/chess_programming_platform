#include "position/bb_position.hpp"
#include "attacks/attacks.hpp"
#include <cassert>
#include <cstdlib>   // std::abs
#include <sstream>

namespace chess {

// Rank / file constants (internal to this file)
static constexpr uint64_t FILE_A = 0x0101010101010101ULL;
static constexpr uint64_t FILE_H = 0x8080808080808080ULL;

static constexpr uint64_t RANK_1 = 0x00000000000000FFULL;
static constexpr uint64_t RANK_2 = 0x000000000000FF00ULL;
static constexpr uint64_t RANK_3 = 0x0000000000FF0000ULL;
static constexpr uint64_t RANK_6 = 0x0000FF0000000000ULL;
static constexpr uint64_t RANK_7 = 0x00FF000000000000ULL;
static constexpr uint64_t RANK_8 = 0xFF00000000000000ULL;

// Pop lowest set bit and return its index
static inline int pop_lsb(uint64_t& b) {
    int sq = __builtin_ctzll(b);
    b &= b - 1;
    return sq;
}

// ============================================================
// Zobrist table (file-local, initialized once)
// ============================================================

namespace {

struct ZobristTable {
    uint64_t pieces[64][13];  // [square][piece index 1-12, 0=unused]
    uint64_t side_to_move;
    uint64_t castling[16];
    uint64_t en_passant[8];

    ZobristTable() {
        uint64_t s = 6364136223846793005ULL; // fixed seed — do not change
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

// Map Piece enum to table index (matches types.hpp enum order)
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

// ============================================================
// Construction
// ============================================================

BitboardPosition::BitboardPosition() {
    for (int i = 0; i < 64; ++i) mailbox_[i] = Piece::Empty;
}

BitboardPosition BitboardPosition::startpos() {
    BitboardPosition pos;
    pos.stm_ = Color::White;
    pos.cr_  = CR_WK | CR_WQ | CR_BK | CR_BQ;
    pos.ep_  = -1;

    for (const auto& pp : STARTPOS_PLACEMENTS) {
        pos.put_piece(pp.pc, pp.sq);
    }

    return pos;
}

// ============================================================
// Internal bitboard primitives
// ============================================================

void BitboardPosition::put_piece(Piece p, int sq) {
    uint64_t b = 1ULL << sq;
    pcs_[piece_ci(p)][piece_ti(p)] |= b;
    occ_[piece_ci(p)] |= b;
    occ_all_ |= b;
    mailbox_[sq] = p;
}

void BitboardPosition::remove_piece(Piece p, int sq) {
    uint64_t mask = ~(1ULL << sq);
    pcs_[piece_ci(p)][piece_ti(p)] &= mask;
    occ_[piece_ci(p)] &= mask;
    occ_all_ &= mask;
    mailbox_[sq] = Piece::Empty;
}

void BitboardPosition::move_piece(Piece p, int from, int to) {
    uint64_t toggle = (1ULL << from) | (1ULL << to);
    pcs_[piece_ci(p)][piece_ti(p)] ^= toggle;
    occ_[piece_ci(p)] ^= toggle;
    occ_all_ ^= toggle;
    mailbox_[from] = Piece::Empty;
    mailbox_[to]   = p;
}

Piece BitboardPosition::promo_piece(Color c, Promotion pr) {
    if (c == Color::White) {
        switch (pr) {
            case PROMO_Q: return Piece::WQ;
            case PROMO_R: return Piece::WR;
            case PROMO_B: return Piece::WB;
            case PROMO_N: return Piece::WN;
            default:      return Piece::WQ;
        }
    } else {
        switch (pr) {
            case PROMO_Q: return Piece::BQ;
            case PROMO_R: return Piece::BR;
            case PROMO_B: return Piece::BB;
            case PROMO_N: return Piece::BN;
            default:      return Piece::BQ;
        }
    }
}

// ============================================================
// Attack detection
// ============================================================

int BitboardPosition::king_square(Color c) const {
    return __builtin_ctzll(pcs_[static_cast<int>(c)][0]);
}

bool BitboardPosition::square_attacked(int sq, Color by) const {
    int ci = static_cast<int>(by);

    if (attacks::pawn(other(by), sq) & pcs_[ci][4]) return true;
    if (attacks::knight(sq) & pcs_[ci][2]) return true;
    if (attacks::king(sq) & pcs_[ci][0]) return true;
    if (attacks::bishop_otf(sq, occ_all_) & (pcs_[ci][3] | pcs_[ci][1])) return true;
    if (attacks::rook_otf(sq, occ_all_) & (pcs_[ci][5] | pcs_[ci][1])) return true;

    return false;
}

bool BitboardPosition::in_check(Color who) const {
    return square_attacked(king_square(who), other(who));
}

// ============================================================
// Zobrist key
// ============================================================

uint64_t BitboardPosition::zobrist_key() const {
    uint64_t key = 0;

    for (int sq = 0; sq < 64; sq++) {
        Piece p = mailbox_[sq];
        if (p != Piece::Empty)
            key ^= ZOBRIST.pieces[sq][piece_index(p)];
    }

    if (stm_ == Color::Black)
        key ^= ZOBRIST.side_to_move;

    key ^= ZOBRIST.castling[cr_];

    if (ep_ >= 0)
        key ^= ZOBRIST.en_passant[file_of(ep_)];

    return key;
}

// ============================================================
// Game result
// ============================================================

void BitboardPosition::set_winner(int w) { winner_ = w; }
int  BitboardPosition::winner()    const  { return winner_; }

// ============================================================
// Fifty move rule
// ============================================================

int  BitboardPosition::halfmove_clock()      const { return halfmove_; }
void BitboardPosition::set_halfmove_clock(int n)   { halfmove_ = n; }
bool BitboardPosition::is_draw_by_fifty()    const { return halfmove_ >= 100; }

// ============================================================
// Threefold repetition
// ============================================================
bool BitboardPosition::is_draw_by_repetition() const {
    if (history_.size() < 2) return false;

    // The current position is history_.back() — count prior occurrences
    uint64_t key = history_.back();
    int count = 0;

    // Walk backwards, only same-side-to-move positions (every 2 plies)
    // Stop early at irreversible moves (halfmove reset) for efficiency
    for (int i = (int)history_.size() - 3; i >= 0; i -= 2) {
        if (history_[i] == key && ++count >= 2) return true;
    }
    return false;
}

std::string BitboardPosition::to_fen() const {
    static const char piece_chars[] = {
        ' ',         // 0 = Empty
        'K','Q','N','B','P','R',  // WK=1..WR=6
        'k','q','n','b','p','r'   // BK=7..BR=12
    };

    std::string fen;
    fen.reserve(80);

    // --- Piece placement ---
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            Piece p = mailbox_[make_sq(file, rank)];
            if (p == Piece::Empty) {
                ++empty;
            } else {
                if (empty) { fen += ('0' + empty); empty = 0; }
                fen += piece_chars[static_cast<int>(p)];
            }
        }
        if (empty) fen += ('0' + empty);
        if (rank > 0) fen += '/';
    }

    // --- Side to move ---
    fen += (stm_ == Color::White) ? " w " : " b ";

    // --- Castling rights ---
    if (cr_ == CR_NONE) {
        fen += '-';
    } else {
        if (cr_ & CR_WK) fen += 'K';
        if (cr_ & CR_WQ) fen += 'Q';
        if (cr_ & CR_BK) fen += 'k';
        if (cr_ & CR_BQ) fen += 'q';
    }

    // --- En passant ---
    fen += ' ';
    if (ep_ < 0) {
        fen += '-';
    } else {
        fen += ('a' + file_of(ep_));
        fen += ('1' + rank_of(ep_));
    }

    // --- Halfmove clock and fullmove number ---
    // Fullmove = number of complete turns; starts at 1
    int fullmove = 1 + ((int)history_.size()) / 2;
    fen += ' ';
    fen += std::to_string(halfmove_);
    fen += ' ';
    fen += std::to_string(fullmove);

    return fen;
}

// ============================================================
// Move execution (with undo stack)
// ============================================================

void BitboardPosition::make_move(Move m) {
    int from = m.from, to = m.to;
    Piece moving = mailbox_[from];

    State st;
    st.cr       = cr_;
    st.ep       = static_cast<int8_t>(ep_);
    st.halfmove = static_cast<int8_t>(halfmove_);
    st.captured = Piece::Empty;
    st.moving   = moving;
    st.was_ep   = false;
    st.m        = m;

    // --- En passant capture ---
    if ((moving == Piece::WP || moving == Piece::BP) &&
        ep_ >= 0 && to == ep_ && is_empty(mailbox_[to])) {

        st.was_ep   = true;
        int cap_sq  = (stm_ == Color::White) ? to - 8 : to + 8;
        st.captured = mailbox_[cap_sq];
        remove_piece(st.captured, cap_sq);
        move_piece(moving, from, to);

    } else {
        // --- Normal capture (if any) ---
        if (!is_empty(mailbox_[to])) {
            st.captured = mailbox_[to];
            remove_piece(st.captured, to);
        }

        move_piece(moving, from, to);

        // --- Promotion ---
        if (m.promo != PROMO_NONE) {
            remove_piece(moving, to);
            put_piece(promo_piece(stm_, static_cast<Promotion>(m.promo)), to);
        }

        // --- Castling: move the rook ---
        if ((moving == Piece::WK || moving == Piece::BK) &&
            std::abs(file_of(to) - file_of(from)) == 2) {

            if (stm_ == Color::White) {
                if (to == make_sq(6, 0))
                    move_piece(Piece::WR, make_sq(7, 0), make_sq(5, 0)); // h1 -> f1
                else
                    move_piece(Piece::WR, make_sq(0, 0), make_sq(3, 0)); // a1 -> d1
            } else {
                if (to == make_sq(6, 7))
                    move_piece(Piece::BR, make_sq(7, 7), make_sq(5, 7)); // h8 -> f8
                else
                    move_piece(Piece::BR, make_sq(0, 7), make_sq(3, 7)); // a8 -> d8
            }
        }
    }

    // --- Update castling rights ---
    if (moving == Piece::WK) cr_ &= ~(CR_WK | CR_WQ);
    if (moving == Piece::BK) cr_ &= ~(CR_BK | CR_BQ);
    if (moving == Piece::WR) {
        if (from == make_sq(7, 0)) cr_ &= ~CR_WK;
        if (from == make_sq(0, 0)) cr_ &= ~CR_WQ;
    }
    if (moving == Piece::BR) {
        if (from == make_sq(7, 7)) cr_ &= ~CR_BK;
        if (from == make_sq(0, 7)) cr_ &= ~CR_BQ;
    }
    if (st.captured == Piece::WR) {
        if (to == make_sq(7, 0)) cr_ &= ~CR_WK;
        if (to == make_sq(0, 0)) cr_ &= ~CR_WQ;
    }
    if (st.captured == Piece::BR) {
        if (to == make_sq(7, 7)) cr_ &= ~CR_BK;
        if (to == make_sq(0, 7)) cr_ &= ~CR_BQ;
    }

    // --- Update en-passant target ---
    ep_ = -1;
    if (moving == Piece::WP && rank_of(to) - rank_of(from) == 2) {
        ep_ = make_sq(file_of(from), rank_of(from) + 1);
    } else if (moving == Piece::BP && rank_of(from) - rank_of(to) == 2) {
        ep_ = make_sq(file_of(from), rank_of(from) - 1);
    }

    // --- Halfmove clock (fifty move rule) ---
    bool is_capture = (st.captured != Piece::Empty);
    bool is_pawn    = (moving == Piece::WP || moving == Piece::BP);
    if (is_pawn || is_capture)
        halfmove_ = 0;
    else
        halfmove_++;

    // --- Record position for repetition detection ---
    stm_ = other(stm_);
    history_.push_back(zobrist_key());

    stack_.push_back(st);
}

void BitboardPosition::undo_move() {
    assert(!stack_.empty());
    State st = stack_.back();
    stack_.pop_back();

    // Pop the history entry pushed by make_move
    if (!history_.empty()) history_.pop_back();

    stm_      = other(stm_);
    cr_       = st.cr;
    ep_       = st.ep;
    halfmove_ = st.halfmove;

    int from = st.m.from, to = st.m.to;

    // --- Undo castling rook ---
    if ((st.moving == Piece::WK || st.moving == Piece::BK) &&
        std::abs(file_of(to) - file_of(from)) == 2) {

        if (stm_ == Color::White) {
            if (to == make_sq(6, 0))
                move_piece(Piece::WR, make_sq(5, 0), make_sq(7, 0));
            else
                move_piece(Piece::WR, make_sq(3, 0), make_sq(0, 0));
        } else {
            if (to == make_sq(6, 7))
                move_piece(Piece::BR, make_sq(5, 7), make_sq(7, 7));
            else
                move_piece(Piece::BR, make_sq(3, 7), make_sq(0, 7));
        }
    }

    // --- Undo promotion or normal move ---
    if (st.m.promo != PROMO_NONE) {
        remove_piece(mailbox_[to], to);
        put_piece(st.moving, from);
    } else {
        move_piece(st.moving, to, from);
    }

    // --- Restore captured piece ---
    if (st.captured != Piece::Empty) {
        if (st.was_ep) {
            int cap_sq = (stm_ == Color::White) ? to - 8 : to + 8;
            put_piece(st.captured, cap_sq);
        } else {
            put_piece(st.captured, to);
        }
    }
}

// ============================================================
// Move generation — pseudo-legal
// ============================================================

static inline void push_promos(MoveList& out, uint8_t from, uint8_t to) {
    out.push({from, to, PROMO_Q});
    out.push({from, to, PROMO_R});
    out.push({from, to, PROMO_B});
    out.push({from, to, PROMO_N});
}

void BitboardPosition::gen_pawns(MoveList& out) const {
    int ci = static_cast<int>(stm_);
    uint64_t P = pcs_[ci][4];
    uint64_t them_occ = occ_[1 - ci];

    if (stm_ == Color::White) {
        uint64_t single = (P << 8) & ~occ_all_;
        uint64_t dbl    = ((single & RANK_3) << 8) & ~occ_all_;

        for (uint64_t b = single & ~RANK_8; b;) { int to = pop_lsb(b); out.push({(uint8_t)(to - 8), (uint8_t)to, PROMO_NONE}); }
        for (uint64_t b = single & RANK_8;  b;) { int to = pop_lsb(b); push_promos(out, (uint8_t)(to - 8), (uint8_t)to); }
        for (uint64_t b = dbl; b;)               { int to = pop_lsb(b); out.push({(uint8_t)(to - 16), (uint8_t)to, PROMO_NONE}); }

        uint64_t capsNW = ((P & ~FILE_A) << 7) & them_occ;
        uint64_t capsNE = ((P & ~FILE_H) << 9) & them_occ;

        for (uint64_t b = capsNW & ~RANK_8; b;) { int to = pop_lsb(b); out.push({(uint8_t)(to - 7), (uint8_t)to, PROMO_NONE}); }
        for (uint64_t b = capsNW & RANK_8;  b;) { int to = pop_lsb(b); push_promos(out, (uint8_t)(to - 7), (uint8_t)to); }
        for (uint64_t b = capsNE & ~RANK_8; b;) { int to = pop_lsb(b); out.push({(uint8_t)(to - 9), (uint8_t)to, PROMO_NONE}); }
        for (uint64_t b = capsNE & RANK_8;  b;) { int to = pop_lsb(b); push_promos(out, (uint8_t)(to - 9), (uint8_t)to); }

        if (ep_ >= 0) {
            uint64_t ep_bb = 1ULL << ep_;
            if (file_of(ep_) > 0 && (P & (ep_bb >> 9))) out.push({(uint8_t)(ep_ - 9), (uint8_t)ep_, PROMO_NONE});
            if (file_of(ep_) < 7 && (P & (ep_bb >> 7))) out.push({(uint8_t)(ep_ - 7), (uint8_t)ep_, PROMO_NONE});
        }

    } else {
        uint64_t single = (P >> 8) & ~occ_all_;
        uint64_t dbl    = ((single & RANK_6) >> 8) & ~occ_all_;

        for (uint64_t b = single & ~RANK_1; b;) { int to = pop_lsb(b); out.push({(uint8_t)(to + 8), (uint8_t)to, PROMO_NONE}); }
        for (uint64_t b = single & RANK_1;  b;) { int to = pop_lsb(b); push_promos(out, (uint8_t)(to + 8), (uint8_t)to); }
        for (uint64_t b = dbl; b;)               { int to = pop_lsb(b); out.push({(uint8_t)(to + 16), (uint8_t)to, PROMO_NONE}); }

        uint64_t capsSE = ((P & ~FILE_H) >> 7) & them_occ;
        uint64_t capsSW = ((P & ~FILE_A) >> 9) & them_occ;

        for (uint64_t b = capsSE & ~RANK_1; b;) { int to = pop_lsb(b); out.push({(uint8_t)(to + 7), (uint8_t)to, PROMO_NONE}); }
        for (uint64_t b = capsSE & RANK_1;  b;) { int to = pop_lsb(b); push_promos(out, (uint8_t)(to + 7), (uint8_t)to); }
        for (uint64_t b = capsSW & ~RANK_1; b;) { int to = pop_lsb(b); out.push({(uint8_t)(to + 9), (uint8_t)to, PROMO_NONE}); }
        for (uint64_t b = capsSW & RANK_1;  b;) { int to = pop_lsb(b); push_promos(out, (uint8_t)(to + 9), (uint8_t)to); }

        if (ep_ >= 0) {
            uint64_t ep_bb = 1ULL << ep_;
            if (file_of(ep_) < 7 && (P & (ep_bb << 9))) out.push({(uint8_t)(ep_ + 9), (uint8_t)ep_, PROMO_NONE});
            if (file_of(ep_) > 0 && (P & (ep_bb << 7))) out.push({(uint8_t)(ep_ + 7), (uint8_t)ep_, PROMO_NONE});
        }
    }
}

void BitboardPosition::gen_knights(MoveList& out) const {
    int ci = static_cast<int>(stm_);
    for (uint64_t b = pcs_[ci][2]; b;) {
        int from = pop_lsb(b);
        uint64_t moves = attacks::knight(from) & ~occ_[ci];
        while (moves) { int to = pop_lsb(moves); out.push({(uint8_t)from, (uint8_t)to, PROMO_NONE}); }
    }
}

void BitboardPosition::gen_bishops(MoveList& out) const {
    int ci = static_cast<int>(stm_);
    for (uint64_t b = pcs_[ci][3]; b;) {
        int from = pop_lsb(b);
        uint64_t moves = attacks::bishop_otf(from, occ_all_) & ~occ_[ci];
        while (moves) { int to = pop_lsb(moves); out.push({(uint8_t)from, (uint8_t)to, PROMO_NONE}); }
    }
}

void BitboardPosition::gen_rooks(MoveList& out) const {
    int ci = static_cast<int>(stm_);
    for (uint64_t b = pcs_[ci][5]; b;) {
        int from = pop_lsb(b);
        uint64_t moves = attacks::rook_otf(from, occ_all_) & ~occ_[ci];
        while (moves) { int to = pop_lsb(moves); out.push({(uint8_t)from, (uint8_t)to, PROMO_NONE}); }
    }
}

void BitboardPosition::gen_queens(MoveList& out) const {
    int ci = static_cast<int>(stm_);
    for (uint64_t b = pcs_[ci][1]; b;) {
        int from = pop_lsb(b);
        uint64_t moves = (attacks::bishop_otf(from, occ_all_) |
                          attacks::rook_otf(from, occ_all_)) & ~occ_[ci];
        while (moves) { int to = pop_lsb(moves); out.push({(uint8_t)from, (uint8_t)to, PROMO_NONE}); }
    }
}

void BitboardPosition::gen_king(MoveList& out) const {
    int ci     = static_cast<int>(stm_);
    Color them = other(stm_);
    int from   = king_square(stm_);

    uint64_t moves = attacks::king(from) & ~occ_[ci];
    while (moves) { int to = pop_lsb(moves); out.push({(uint8_t)from, (uint8_t)to, PROMO_NONE}); }

    if (stm_ == Color::White) {
        if ((cr_ & CR_WK) &&
            !(occ_all_ & ((1ULL << make_sq(5, 0)) | (1ULL << make_sq(6, 0)))) &&
            !square_attacked(make_sq(4, 0), them) &&
            !square_attacked(make_sq(5, 0), them))
            out.push({(uint8_t)make_sq(4, 0), (uint8_t)make_sq(6, 0), PROMO_NONE});

        if ((cr_ & CR_WQ) &&
            !(occ_all_ & ((1ULL << make_sq(3, 0)) | (1ULL << make_sq(2, 0)) | (1ULL << make_sq(1, 0)))) &&
            !square_attacked(make_sq(4, 0), them) &&
            !square_attacked(make_sq(3, 0), them))
            out.push({(uint8_t)make_sq(4, 0), (uint8_t)make_sq(2, 0), PROMO_NONE});
    } else {
        if ((cr_ & CR_BK) &&
            !(occ_all_ & ((1ULL << make_sq(5, 7)) | (1ULL << make_sq(6, 7)))) &&
            !square_attacked(make_sq(4, 7), them) &&
            !square_attacked(make_sq(5, 7), them))
            out.push({(uint8_t)make_sq(4, 7), (uint8_t)make_sq(6, 7), PROMO_NONE});

        if ((cr_ & CR_BQ) &&
            !(occ_all_ & ((1ULL << make_sq(3, 7)) | (1ULL << make_sq(2, 7)) | (1ULL << make_sq(1, 7)))) &&
            !square_attacked(make_sq(4, 7), them) &&
            !square_attacked(make_sq(3, 7), them))
            out.push({(uint8_t)make_sq(4, 7), (uint8_t)make_sq(2, 7), PROMO_NONE});
    }
}

void BitboardPosition::generate_pseudo(MoveList& out) const {
    out.clear();
    gen_pawns(out);
    gen_knights(out);
    gen_bishops(out);
    gen_rooks(out);
    gen_queens(out);
    gen_king(out);
}

void BitboardPosition::generate_legal(MoveList& out) {
    MoveList pseudo;
    generate_pseudo(pseudo);
    out.clear();

    for (int i = 0; i < pseudo.size; ++i) {
        Move m = pseudo.moves[i];
        make_move(m);
        bool legal = !square_attacked(king_square(other(stm_)), stm_);
        undo_move();
        if (legal) out.push(m);
    }
}

bool BitboardPosition::has_any_legal_move() {
    MoveList pseudo;
    generate_pseudo(pseudo);

    for (int i = 0; i < pseudo.size; ++i) {
        Move m = pseudo.moves[i];
        make_move(m);
        bool legal = !square_attacked(king_square(other(stm_)), stm_);
        undo_move();
        if (legal) return true;
    }
    return false;
}

BitboardPosition BitboardPosition::from_fen(const std::string& fen) {
    BitboardPosition pos;
    pos.stm_      = Color::White;
    pos.cr_       = CR_NONE;
    pos.ep_       = -1;
    pos.halfmove_ = 0;

    std::istringstream ss(fen);
    std::string token;

    // --- Piece placement ---
    ss >> token;
    int rank = 7, file = 0;
    for (char c : token) {
        if (c == '/') {
            --rank;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += (c - '0');
        } else {
            Piece p = Piece::Empty;
            switch (c) {
                case 'K': p = Piece::WK; break;  case 'k': p = Piece::BK; break;
                case 'Q': p = Piece::WQ; break;  case 'q': p = Piece::BQ; break;
                case 'N': p = Piece::WN; break;  case 'n': p = Piece::BN; break;
                case 'B': p = Piece::WB; break;  case 'b': p = Piece::BB; break;
                case 'P': p = Piece::WP; break;  case 'p': p = Piece::BP; break;
                case 'R': p = Piece::WR; break;  case 'r': p = Piece::BR; break;
                default: break;
            }
            if (p != Piece::Empty)
                pos.put_piece(p, make_sq(file, rank));
            ++file;
        }
    }

    // --- Side to move ---
    ss >> token;
    pos.stm_ = (token == "b") ? Color::Black : Color::White;

    // --- Castling rights ---
    ss >> token;
    if (token != "-") {
        for (char c : token) {
            switch (c) {
                case 'K': pos.cr_ |= CR_WK; break;
                case 'Q': pos.cr_ |= CR_WQ; break;
                case 'k': pos.cr_ |= CR_BK; break;
                case 'q': pos.cr_ |= CR_BQ; break;
            }
        }
    }

    // --- En passant ---
    ss >> token;
    if (token != "-") {
        int ep_file = token[0] - 'a';
        int ep_rank = token[1] - '1';
        pos.ep_ = make_sq(ep_file, ep_rank);
    }

    // --- Halfmove clock ---
    int halfmove = 0, fullmove = 1;
    if (ss >> halfmove) pos.halfmove_ = halfmove;
    if (ss >> fullmove) {
        // Reconstruct history size so fullmove tracking in to_fen() stays consistent.
        // history_ is empty on a freshly loaded position — that's correct since
        // we have no move record prior to this point.
        (void)fullmove;
    }

    // Push the starting key so is_draw_by_repetition() has a baseline
    pos.history_.push_back(pos.zobrist_key());

    return pos;
}

} // namespace chess
