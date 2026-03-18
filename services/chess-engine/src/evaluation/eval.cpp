#include "evaluation/eval.hpp"

namespace chess {

static int value(Piece p) {
    switch (p) {
        case Piece::WP: return 100;
        case Piece::WN: return 320;
        case Piece::WB: return 330;
        case Piece::WR: return 500;
        case Piece::WQ: return 900;
        case Piece::WK: return 0;

        case Piece::BP: return -100;
        case Piece::BN: return -320;
        case Piece::BB: return -330;
        case Piece::BR: return -500;
        case Piece::BQ: return -900;
        case Piece::BK: return 0;

        default: return 0;
    }
}

int Eval::evaluate(const Position& pos) {
    int score = 0;

    for (int sq = 0; sq < 64; ++sq) {
        score += value(pos.at(sq));
    }

    return score;
}

} // namespace chess