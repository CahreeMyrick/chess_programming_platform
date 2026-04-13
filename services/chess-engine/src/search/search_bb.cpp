#include "search/search_bb.hpp"
#include "evaluation/eval.hpp"
#include <limits>
#include <algorithm>
#include <array>

namespace chess {

const int CONTEMPT = 25;

// ── Transposition Table ───────────────────────────────────────────────────────

enum class TTFlag : uint8_t {
    EXACT,      // exact score
    LOWERBOUND, // alpha cutoff — score is at least this
    UPPERBOUND  // beta cutoff — score is at most this
};

struct TTEntry {
    uint64_t key   = 0;
    int      score = 0;
    int      depth = -1;
    TTFlag   flag  = TTFlag::EXACT;
    Move     best  = {};
};

// 1 << 22 = 4M entries × ~24 bytes = ~96MB — fits comfortably in M3 unified memory
static constexpr size_t TT_SIZE = 1 << 22;
static std::array<TTEntry, TT_SIZE> tt;

static void tt_clear() {
    tt.fill(TTEntry{});
}

static TTEntry* tt_probe(uint64_t key) {
    TTEntry& entry = tt[key & (TT_SIZE - 1)];
    return (entry.key == key) ? &entry : nullptr;
}

static void tt_store(uint64_t key, int score, int depth, TTFlag flag, Move best) {
    TTEntry& entry = tt[key & (TT_SIZE - 1)];
    // Always replace if depth is greater or equal (depth-preferred replacement)
    if (entry.key == 0 || depth >= entry.depth) {
        entry = { key, score, depth, flag, best };
    }
}

// ── Move ordering ─────────────────────────────────────────────────────────────

static auto piece_val = [](Piece p) -> int {
    switch (p) {
        case Piece::WQ: case Piece::BQ: return 900;
        case Piece::WR: case Piece::BR: return 500;
        case Piece::WB: case Piece::BB: return 300;
        case Piece::WN: case Piece::BN: return 300;
        case Piece::WP: case Piece::BP: return 100;
        default:                        return 0;
    }
};

static void order_moves(BitboardPosition& pos, MoveList& moves, Move tt_best = {}) {
    auto move_score = [&](const Move& m) -> int {
        // TT best move searched first
        if (m.from == tt_best.from && m.to == tt_best.to && m.promo == tt_best.promo)
            return 100000;

        int score = 0;
        Piece captured = pos.piece_at(m.to);
        Piece moving   = pos.piece_at(m.from);

        // MVV-LVA
        if (captured != Piece::Empty)
            score += 10000 + piece_val(captured) - piece_val(moving);

        // Promotions
        if (m.promo != PROMO_NONE) score += 8000;

        return score;
    };

    std::sort(moves.moves.begin(), moves.moves.begin() + moves.size,
          [&](const Move& a, const Move& b) {
              return move_score(a) > move_score(b);
          });
}

// ── Quiescence search ─────────────────────────────────────────────────────────

static int quiesce(BitboardPosition& pos, int alpha, int beta) {
    int stand_pat = Eval::evaluate(pos);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList moves;
    pos.generate_legal(moves);
    order_moves(pos, moves);

    for (int i = 0; i < moves.size; ++i) {
        if (pos.piece_at(moves.moves[i].to) == Piece::Empty) continue;

        pos.make_move(moves.moves[i]);
        int score = quiesce(pos, -beta, -alpha);
        pos.undo_move();

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

// ── Alpha-beta ────────────────────────────────────────────────────────────────

static int alphabeta_bb(BitboardPosition& pos, int depth, int alpha, int beta, int ply) {
    if (pos.is_draw_by_repetition())
        return (pos.side_to_move() == Color::White) ? -CONTEMPT : +CONTEMPT;
    if (pos.is_draw_by_fifty()) return 0;

    uint64_t key = pos.zobrist_key();
    int orig_alpha = alpha;
    Move tt_best = {};

    // --- TT lookup ---
    TTEntry* entry = tt_probe(key);
    if (entry && entry->depth >= depth) {
        switch (entry->flag) {
            case TTFlag::EXACT:
                return entry->score;
            case TTFlag::LOWERBOUND:
                alpha = std::max(alpha, entry->score);
                break;
            case TTFlag::UPPERBOUND:
                beta = std::min(beta, entry->score);
                break;
        }
        if (alpha >= beta) return entry->score;
        tt_best = entry->best;
    } else if (entry) {
        tt_best = entry->best; // use best move for ordering even if depth insufficient
    }

    if (depth == 0) {
        return quiesce(pos, alpha, beta);
    }

    MoveList moves;
    pos.generate_legal(moves);
    order_moves(pos, moves, tt_best);

    if (moves.size == 0) {
        if (pos.in_check(pos.side_to_move()))
            return (pos.side_to_move() == Color::White)
                ? -100000 + ply
                : +100000 - ply;
        return 0;
    }

    Move best_move = moves.moves[0];

    if (pos.side_to_move() == Color::White) {
        int best = std::numeric_limits<int>::min();
        for (int i = 0; i < moves.size; ++i) {
            pos.make_move(moves.moves[i]);
            int score = alphabeta_bb(pos, depth - 1, alpha, beta, ply + 1);
            pos.undo_move();

            if (score > best) { best = score; best_move = moves.moves[i]; }
            alpha = std::max(alpha, score);
            if (beta <= alpha) break;
        }

        // Store in TT
        TTFlag flag = (best <= orig_alpha) ? TTFlag::UPPERBOUND
                    : (best >= beta)       ? TTFlag::LOWERBOUND
                                           : TTFlag::EXACT;
        tt_store(key, best, depth, flag, best_move);
        return best;

    } else {
        int best = std::numeric_limits<int>::max();
        for (int i = 0; i < moves.size; ++i) {
            pos.make_move(moves.moves[i]);
            int score = alphabeta_bb(pos, depth - 1, alpha, beta, ply + 1);
            pos.undo_move();

            if (score < best) { best = score; best_move = moves.moves[i]; }
            beta = std::min(beta, score);
            if (beta <= alpha) break;
        }

        TTFlag flag = (best >= beta)       ? TTFlag::LOWERBOUND
                    : (best <= orig_alpha) ? TTFlag::UPPERBOUND
                                           : TTFlag::EXACT;
        tt_store(key, best, depth, flag, best_move);
        return best;
    }
}

// ── Root ──────────────────────────────────────────────────────────────────────

SearchResult SearchBB::minimax(BitboardPosition& pos, int depth) {
    MoveList moves;
    pos.generate_legal(moves);

    // Pull TT best move for root ordering
    Move tt_best = {};
    TTEntry* entry = tt_probe(pos.zobrist_key());
    if (entry) tt_best = entry->best;

    order_moves(pos, moves, tt_best);

    Color root_stm = pos.side_to_move();

    SearchResult res;
    res.score = (root_stm == Color::White)
        ? std::numeric_limits<int>::min()
        : std::numeric_limits<int>::max();
    res.best = moves.moves[0];

    for (int i = 0; i < moves.size; ++i) {
        pos.make_move(moves.moves[i]);
        int score = alphabeta_bb(
            pos, depth - 1,
            std::numeric_limits<int>::min(),
            std::numeric_limits<int>::max(),
            1
        );
        pos.undo_move();

        if (root_stm == Color::White) {
            if (score > res.score) { res.score = score; res.best = moves.moves[i]; }
        } else {
            if (score < res.score) { res.score = score; res.best = moves.moves[i]; }
        }
    }
    return res;
}

// ── Public TT control ─────────────────────────────────────────────────────────

void SearchBB::clear_tt() {
    tt_clear();
}

} // namespace chess