#include <iostream>
#include <memory>
#include <string>
#include <algorithm>
#include "position/board_factory.hpp"
#include "position/position.hpp"
#include "render/render.hpp"
#include "search/search.hpp"
#include "movegen/movegen.hpp"
#include "rules/rules.hpp"
#include "core/types.hpp"
#include "core/move.hpp"

namespace chess {

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string square_to_string(int sq) {
    char file = static_cast<char>('a' + file_of(sq));
    char rank = static_cast<char>('1' + rank_of(sq));
    std::string out;
    out.push_back(file);
    out.push_back(rank);
    return out;
}

static std::string move_to_string(const Move& m) {
    std::string s = square_to_string(m.from) + square_to_string(m.to);
    if (m.promo != PROMO_NONE) {
        switch (m.promo) {
            case PROMO_Q: s += 'q'; break;
            case PROMO_R: s += 'r'; break;
            case PROMO_B: s += 'b'; break;
            case PROMO_N: s += 'n'; break;
            default:                break;
        }
    }
    return s;
}

// Parse "e2e4" or "e7e8q" into a Move. Returns false if malformed.
static bool parse_move(const std::string& s, Move& out) {
    if (s.size() < 4)             return false;
    if (s[0] < 'a' || s[0] > 'h') return false;
    if (s[2] < 'a' || s[2] > 'h') return false;
    if (s[1] < '1' || s[1] > '8') return false;
    if (s[3] < '1' || s[3] > '8') return false;

    int from_file = s[0] - 'a';
    int from_rank = s[1] - '1';
    int to_file   = s[2] - 'a';
    int to_rank   = s[3] - '1';

    out.from  = static_cast<uint8_t>(from_rank * 8 + from_file);
    out.to    = static_cast<uint8_t>(to_rank   * 8 + to_file);
    out.promo = PROMO_NONE;

    if (s.size() >= 5) {
        switch (s[4]) {
            case 'q': out.promo = PROMO_Q; break;
            case 'r': out.promo = PROMO_R; break;
            case 'b': out.promo = PROMO_B; break;
            case 'n': out.promo = PROMO_N; break;
            default: return false;
        }
    }
    return true;
}

static const char* board_type_name(BoardType type) {
    switch (type) {
        case BoardType::Array:    return "ArrayBoard";
        case BoardType::Pointer:  return "PointerBoard";
        case BoardType::Bitboard: return "Bitboard";
        default:                  return "Unknown";
    }
}

static BoardType parse_board_type(const std::string& s) {
    if (s == "array")    return BoardType::Array;
    if (s == "pointer")  return BoardType::Pointer;
    if (s == "bitboard") return BoardType::Bitboard;
    return BoardType::Array;
}

// ── Single-search mode (original behaviour) ───────────────────────────────────

static void run_search(Position& pos, BoardType type) {
    std::cout << "Board backend: " << board_type_name(type) << "\n";
    std::cout << "Initial position:\n";
    std::cout << Render::board_ascii(pos) << "\n";

    int depth = 3;
    SearchResult result = Search::minimax(pos, depth);
    std::cout << "Search depth: " << depth << "\n";
    std::cout << "Best move:    " << move_to_string(result.best) << "\n";
    std::cout << "Score:        " << result.score << "\n";
}

// ── Interactive play mode ─────────────────────────────────────────────────────

static void run_play(Position& pos, BoardType type, int engine_depth = 3) {
    std::cout << "=== Play vs Engine ===\n";
    std::cout << "Backend: " << board_type_name(type)
              << "  |  Engine depth: " << engine_depth << "\n";
    std::cout << "You play White. Enter moves in UCI format (e.g. e2e4, e7e8q).\n";
    std::cout << "Commands: 'quit' to exit, 'moves' to list legal moves.\n\n";

    while (true) {
        std::cout << Render::board_ascii(pos) << "\n";

        // Generate legal moves for the current side
        MoveList legal;
        MoveGen::generate_legal(pos, legal);

        if (legal.size == 0) {
            if (Rules::in_check(pos, pos.side_to_move()))
                std::cout << "Checkmate! Engine wins.\n";
            else
                std::cout << "Stalemate! Draw.\n";
            break;
        }

        // ── User's turn (White) ───────────────────────────────────────────────
        std::cout << "Your move: ";
        std::string line;
        if (!std::getline(std::cin, line)) break;

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        auto last = line.find_last_not_of(" \t\r\n");
        if (last != std::string::npos) line.erase(last + 1);

        // Normalise to lower-case
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);

        if (line == "quit" || line == "q") break;

        if (line == "moves") {
            std::cout << "Legal moves:";
            for (int i = 0; i < legal.size; ++i)
                std::cout << " " << move_to_string(legal.moves[i]);
            std::cout << "\n\n";
            continue;
        }

        Move user_move{};
        if (!parse_move(line, user_move)) {
            std::cout << "  Unrecognised input. Use UCI format (e.g. e2e4).\n\n";
            continue;
        }

        // Find the matching legal move (validates from, to, and promo)
        const Move* matched = nullptr;
        for (int i = 0; i < legal.size; ++i) {
            const Move& m = legal.moves[i];
            if (m.from == user_move.from && m.to == user_move.to
                    && m.promo == user_move.promo) {
                matched = &m;
                break;
            }
        }

        if (!matched) {
            std::cout << "  Illegal move. Type 'moves' to see legal moves.\n\n";
            continue;
        }

        // Apply via make_move, which handles EP, castling, promotion, and
        // state updates (castling rights, ep square, side to move)
        std::string err;
        if (!pos.make_move(*matched, err)) {
            std::cout << "  Move rejected: " << err << "\n\n";
            continue;
        }

        std::cout << "  You played: " << move_to_string(*matched) << "\n\n";

        // ── Engine's turn (Black) ─────────────────────────────────────────────
        MoveList engine_legal;
        MoveGen::generate_legal(pos, engine_legal);

        if (engine_legal.size == 0) {
            std::cout << Render::board_ascii(pos) << "\n";
            if (Rules::in_check(pos, pos.side_to_move()))
                std::cout << "Checkmate! You win!\n";
            else
                std::cout << "Stalemate! Draw.\n";
            break;
        }

        std::cout << "Engine thinking...\n";
        SearchResult result = Search::minimax(pos, engine_depth);

        std::string err2;
        if (!pos.make_move(result.best, err2)) {
            std::cout << "  Engine move rejected: " << err2 << "\n";
            break;
        }

        std::cout << "Engine played: " << move_to_string(result.best)
                  << "  (score: " << result.score << ")\n\n";
    }

    std::cout << "Thanks for playing!\n";
}

} // namespace chess

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    using namespace chess;

    BoardType type        = BoardType::Array;
    bool      play_mode   = false;
    int       engine_depth = 3;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "play")                 play_mode    = true;
        else if (arg == "array")                type         = BoardType::Array;
        else if (arg == "pointer")              type         = BoardType::Pointer;
        else if (arg == "bitboard")             type         = BoardType::Bitboard;
        else if (arg.rfind("--depth=", 0) == 0) engine_depth = std::stoi(arg.substr(8));
    }

    auto board = make_board(type);
    if (!board) {
        std::cerr << "Selected board type is not implemented.\n";
        return 1;
    }

    Position pos = Position::startpos(std::move(board));

    if (play_mode)
        run_play(pos, type, engine_depth);
    else
        run_search(pos, type);

    return 0;
}