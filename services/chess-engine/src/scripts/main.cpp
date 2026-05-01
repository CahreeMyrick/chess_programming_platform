#include <iostream>
#include <memory>
#include <string>
#include <algorithm>

#include "position/board_factory.hpp"
#include "position/position.hpp"
#include "position/bb_position.hpp"

#include "render/render.hpp"

#include "search/search.hpp"
#include "search/search_bb.hpp"
#include "search/search_nn.hpp"

#include "movegen/movegen.hpp"
#include "rules/rules.hpp"

#include "core/types.hpp"
#include "core/move.hpp"
#include "core/uci.hpp"

#include "selfplay/selfplay.hpp"

namespace chess {

// ── Search Mode ───────────────────────────────────────────────────────────────

enum class SearchMode {
    Generic,
    Bitboard,
    Neural
};

static const char* search_mode_name(SearchMode mode) {
    switch (mode) {
        case SearchMode::Generic:  return "Generic Search";
        case SearchMode::Bitboard: return "Bitboard Search";
        case SearchMode::Neural:   return "Neural Search";
        default:                   return "Unknown Search";
    }
}

static SearchMode parse_search_mode(const std::string& s) {
    if (s == "generic")  return SearchMode::Generic;
    if (s == "bitboard") return SearchMode::Bitboard;
    if (s == "nn")       return SearchMode::Neural;
    if (s == "neural")   return SearchMode::Neural;

    return SearchMode::Generic;
}

// ── Helpers ──────────────────────────────────────────────────────────────────

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
            default: break;
        }
    }

    return s;
}

static bool parse_move(const std::string& s, Move& out) {
    if (s.size() < 4) return false;

    if (s[0] < 'a' || s[0] > 'h') return false;
    if (s[2] < 'a' || s[2] > 'h') return false;
    if (s[1] < '1' || s[1] > '8') return false;
    if (s[3] < '1' || s[3] > '8') return false;

    int from_file = s[0] - 'a';
    int from_rank = s[1] - '1';
    int to_file   = s[2] - 'a';
    int to_rank   = s[3] - '1';

    out.from  = static_cast<uint8_t>(from_rank * 8 + from_file);
    out.to    = static_cast<uint8_t>(to_rank * 8 + to_file);
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

// ── Generic Position Search ──────────────────────────────────────────────────

static void run_search_generic(Position& pos, BoardType type, int depth) {
    std::cout << "Board backend: " << board_type_name(type) << "\n";
    std::cout << "Search mode:   Generic Search\n";
    std::cout << "Initial position:\n";
    std::cout << Render::board_ascii(pos) << "\n";

    SearchResult result = Search::minimax(pos, depth);

    std::cout << "Search depth: " << depth << "\n";
    std::cout << "Best move:    " << move_to_string(result.best) << "\n";
    std::cout << "Score:        " << result.score << "\n";
}

// ── Bitboard Search ──────────────────────────────────────────────────────────

static SearchResult run_bitboard_engine(BitboardPosition& pos,
                                         SearchMode search_mode,
                                         int depth) {
    if (search_mode == SearchMode::Neural) {
        auto nn_result = SearchNN::minimax(pos, depth);

        SearchResult result;
        result.best = nn_result.best;
        result.score = nn_result.score;
        return result;
    }

    return SearchBB::minimax(pos, depth);
}

static void run_search_bitboard(SearchMode search_mode, int depth) {
    BitboardPosition pos = BitboardPosition::startpos();

    std::cout << "Board backend: BitboardPosition\n";
    std::cout << "Search mode:   " << search_mode_name(search_mode) << "\n";
    std::cout << "Initial position:\n";
    std::cout << Render::board_ascii(pos) << "\n";

    SearchResult result = run_bitboard_engine(pos, search_mode, depth);

    std::cout << "Search depth: " << depth << "\n";
    std::cout << "Best move:    " << move_to_uci(result.best) << "\n";
    std::cout << "Score:        " << result.score << "\n";
}

// ── Generic Play Mode ────────────────────────────────────────────────────────

static void run_play_generic(Position& pos, BoardType type, int engine_depth) {
    std::cout << "=== Play vs Engine ===\n";
    std::cout << "Backend: " << board_type_name(type)
              << " | Search: Generic"
              << " | Depth: " << engine_depth << "\n";

    std::cout << "You play White. Enter moves in UCI format, e.g. e2e4.\n";
    std::cout << "Commands: quit, moves\n\n";

    while (true) {
        std::cout << Render::board_ascii(pos) << "\n";

        MoveList legal;
        MoveGen::generate_legal(pos, legal);

        if (legal.size == 0) {
            if (Rules::in_check(pos, pos.side_to_move()))
                std::cout << "Checkmate.\n";
            else
                std::cout << "Stalemate.\n";
            break;
        }

        std::cout << "Your move: ";

        std::string line;
        if (!std::getline(std::cin, line)) break;

        line.erase(0, line.find_first_not_of(" \t\r\n"));
        auto last = line.find_last_not_of(" \t\r\n");
        if (last != std::string::npos) line.erase(last + 1);

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
            std::cout << "Unrecognized input.\n\n";
            continue;
        }

        const Move* matched = nullptr;

        for (int i = 0; i < legal.size; ++i) {
            const Move& m = legal.moves[i];

            if (m.from == user_move.from &&
                m.to == user_move.to &&
                m.promo == user_move.promo) {
                matched = &m;
                break;
            }
        }

        if (!matched) {
            std::cout << "Illegal move.\n\n";
            continue;
        }

        std::string err;
        if (!pos.make_move(*matched, err)) {
            std::cout << "Move rejected: " << err << "\n\n";
            continue;
        }

        std::cout << "You played: " << move_to_string(*matched) << "\n\n";

        MoveList engine_legal;
        MoveGen::generate_legal(pos, engine_legal);

        if (engine_legal.size == 0) {
            std::cout << Render::board_ascii(pos) << "\n";
            if (Rules::in_check(pos, pos.side_to_move()))
                std::cout << "Checkmate! You win.\n";
            else
                std::cout << "Stalemate.\n";
            break;
        }

        std::cout << "Engine thinking...\n";

        SearchResult result = Search::minimax(pos, engine_depth);

        std::string err2;
        if (!pos.make_move(result.best, err2)) {
            std::cout << "Engine move rejected: " << err2 << "\n";
            break;
        }

        std::cout << "Engine played: " << move_to_string(result.best)
                  << " (score: " << result.score << ")\n\n";
    }

    std::cout << "Thanks for playing!\n";
}

// ── Bitboard Play Mode ───────────────────────────────────────────────────────

static void run_play_bitboard(SearchMode search_mode, int engine_depth) {
    BitboardPosition pos = BitboardPosition::startpos();

    std::cout << "=== Play vs Engine ===\n";
    std::cout << "Backend: BitboardPosition"
              << " | Search: " << search_mode_name(search_mode)
              << " | Depth: " << engine_depth << "\n";

    std::cout << "You play White. Enter moves in UCI format, e.g. e2e4.\n";
    std::cout << "Commands: quit, moves\n\n";

    while (true) {
        std::cout << Render::board_ascii(pos) << "\n";

        MoveList legal;
        pos.generate_legal(legal);

        if (legal.size == 0) {
            if (pos.in_check(pos.side_to_move()))
                std::cout << "Checkmate.\n";
            else
                std::cout << "Stalemate.\n";
            break;
        }

        std::cout << "Your move: ";

        std::string line;
        if (!std::getline(std::cin, line)) break;

        line.erase(0, line.find_first_not_of(" \t\r\n"));
        auto last = line.find_last_not_of(" \t\r\n");
        if (last != std::string::npos) line.erase(last + 1);

        std::transform(line.begin(), line.end(), line.begin(), ::tolower);

        if (line == "quit" || line == "q") break;

        if (line == "moves") {
            std::cout << "Legal moves:";
            for (int i = 0; i < legal.size; ++i)
                std::cout << " " << move_to_uci(legal.moves[i]);
            std::cout << "\n\n";
            continue;
        }

        Move user_move{};
        if (!parse_move(line, user_move)) {
            std::cout << "Unrecognized input.\n\n";
            continue;
        }

        const Move* matched = nullptr;

        for (int i = 0; i < legal.size; ++i) {
            const Move& m = legal.moves[i];

            if (m.from == user_move.from &&
                m.to == user_move.to &&
                m.promo == user_move.promo) {
                matched = &m;
                break;
            }
        }

        if (!matched) {
            std::cout << "Illegal move.\n\n";
            continue;
        }

        pos.make_move(*matched);

        std::cout << "You played: " << move_to_uci(*matched) << "\n\n";

        MoveList engine_legal;
        pos.generate_legal(engine_legal);

        if (engine_legal.size == 0) {
            std::cout << Render::board_ascii(pos) << "\n";
            if (pos.in_check(pos.side_to_move()))
                std::cout << "Checkmate! You win.\n";
            else
                std::cout << "Stalemate.\n";
            break;
        }

        std::cout << "Engine thinking...\n";

        SearchResult result = run_bitboard_engine(pos, search_mode, engine_depth);

        pos.make_move(result.best);

        std::cout << "Engine played: " << move_to_uci(result.best)
                  << " (score: " << result.score << ")\n\n";
    }

    std::cout << "Thanks for playing!\n";
}

} // namespace chess

// ── Entry Point ──────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    using namespace chess;

    BoardType type = BoardType::Array;
    SearchMode search_mode = SearchMode::Generic;

    bool play_mode = false;
    int engine_depth = 3;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "play") {
            play_mode = true;
        } else if (arg == "array") {
            type = BoardType::Array;
        } else if (arg == "pointer") {
            type = BoardType::Pointer;
        } else if (arg == "bitboard") {
            type = BoardType::Bitboard;
            search_mode = SearchMode::Bitboard;
        } else if (arg.rfind("--depth=", 0) == 0) {
            engine_depth = std::stoi(arg.substr(8));
        } else if (arg.rfind("--search=", 0) == 0) {
            search_mode = parse_search_mode(arg.substr(9));
        }
    }

    if ((search_mode == SearchMode::Bitboard || search_mode == SearchMode::Neural)
        && type != BoardType::Bitboard) {
        std::cerr << "--search=bitboard or --search=nn requires board type 'bitboard'.\n";
        return 1;
    }

    if (type == BoardType::Bitboard) {
        if (play_mode) {
            run_play_bitboard(search_mode, engine_depth);
        } else {
            run_search_bitboard(search_mode, engine_depth);
        }

        return 0;
    }

    if (search_mode != SearchMode::Generic) {
        std::cerr << "Array/Pointer boards only support --search=generic.\n";
        return 1;
    }

    auto board = make_board(type);

    if (!board) {
        std::cerr << "Selected board type is not implemented.\n";
        return 1;
    }

    Position pos = Position::startpos(std::move(board));

    if (play_mode) {
        run_play_generic(pos, type, engine_depth);
    } else {
        run_search_generic(pos, type, engine_depth);
    }

    return 0;
}