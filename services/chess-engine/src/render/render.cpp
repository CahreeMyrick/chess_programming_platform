#include "render.hpp"
#include <sstream>

namespace chess {

char Render::piece_char(Piece p) {
    switch (p) {
        case Piece::WP: return 'P';
        case Piece::WN: return 'N';
        case Piece::WB: return 'B';
        case Piece::WR: return 'R';
        case Piece::WQ: return 'Q';
        case Piece::WK: return 'K';
        case Piece::BP: return 'p';
        case Piece::BN: return 'n';
        case Piece::BB: return 'b';
        case Piece::BR: return 'r';
        case Piece::BQ: return 'q';
        case Piece::BK: return 'k';
        default:        return '.';
    }
}

std::string Render::board_ascii(const Position& pos) {
    std::ostringstream out;

    out << "\n";
    out << "    a b c d e f g h\n";
    out << "  +-----------------+\n";

    // print ranks 8..1 (rank index 7..0)
    for (int r = 7; r >= 0; --r) {
        out << (r + 1) << " | ";
        for (int f = 0; f < 8; ++f) {
            int sq = make_sq(f, r);
            out << piece_char(pos.at(sq)) << ' ';
        }
        out << "| " << (r + 1) << "\n";
    }

    out << "  +-----------------+\n";
    out << "    a b c d e f g h\n";

    out << "\nSide to move: " << (pos.side_to_move() == Color::White ? "White" : "Black") << "\n";
    return out.str();
}

} // namespace chess

