#include <iostream>
#include <memory>
#include <string>

#include "position/board_factory.hpp"
#include "position/position.hpp"
#include "render/render.hpp"
#include "search/search.hpp"
#include "core/types.hpp"
#include "core/move.hpp"

namespace chess {

static std::string square_to_string(int sq) {
    char file = static_cast<char>('a' + file_of(sq));
    char rank = static_cast<char>('1' + rank_of(sq));

    std::string out;
    out.push_back(file);
    out.push_back(rank);
    return out;
}

static std::string move_to_string(const Move& m) {
    std::string s = square_to_string(m.from) + " " + square_to_string(m.to);

    if (m.promo != PROMO_NONE) {
        char promo_char = '?';
        switch (m.promo) {
            case PROMO_Q: promo_char = 'q'; break;
            case PROMO_R: promo_char = 'r'; break;
            case PROMO_B: promo_char = 'b'; break;
            case PROMO_N: promo_char = 'n'; break;
            default:      promo_char = '?'; break;
        }
        s += " ";
        s.push_back(promo_char);
    }

    return s;
}

static const char* board_type_name(BoardType type) {
    switch (type) {
        case BoardType::Array:   return "ArrayBoard";
        case BoardType::Pointer: return "PointerBoard";
        case BoardType::Bitboard:return "Bitboard";
        default:                 return "Unknown";
    }
}

static BoardType parse_board_type(const std::string& s) {
    if (s == "array")   return BoardType::Array;
    if (s == "pointer") return BoardType::Pointer;
    if (s == "bitboard") return BoardType::Bitboard;
    return BoardType::Array;
}

} // namespace chess

int main(int argc, char* argv[]) {
    using namespace chess;

    BoardType type = BoardType::Array;

    if (argc > 1) {
        type = parse_board_type(argv[1]);
    }

    auto board = make_board(type);
    if (!board) {
        std::cerr << "Selected board type is not implemented.\n";
        return 1;
    }

    Position pos = Position::startpos(std::move(board));

    std::cout << "Board backend: " << board_type_name(type) << "\n";
    std::cout << "Initial position:\n";
    std::cout << Render::board_ascii(pos) << "\n";

    int depth = 3;
    SearchResult result = Search::minimax(pos, depth);

    std::cout << "Search depth: " << depth << "\n";
    std::cout << "Best move: " << move_to_string(result.best) << "\n";
    std::cout << "Score: " << result.score << "\n";

    return 0;
}