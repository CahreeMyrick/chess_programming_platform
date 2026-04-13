#include <iostream>
#include <string>
#include <vector>
#include "position/bb_position.hpp"
#include "render/render.hpp"
#include "search/search_bb.hpp"
#include "core/types.hpp"
#include "core/move.hpp"
#include "openings/opening_book.h"
#include "parsing/parse.hpp"

namespace chess {

// ── Help ──────────────────────────────────────────────────────────────────────

void print_help(const char* program_name) {
    std::cout <<
        "\nUsage: " << program_name << " [options]\n"
        "\nPosition:\n"
        "  standard              Start from the default chess position (default)\n"
        "  book                  Start from a random opening (noob_3moves.epd)\n"
        "\nDepth:\n"
        "  depth N               Set both sides to depth N\n"
        "  wdepth N              Set white's search depth (default: 1)\n"
        "  bdepth N              Set black's search depth (default: 4)\n"
        "\nGames:\n"
        "  num_games N           Number of games to play (default: 1)\n"
        "\nOutput:\n"
        "  quiet                 Suppress board printing, show results only\n"
        "\nOther:\n"
        "  help                  Show this message\n"
        "\nExamples:\n"
        "  " << program_name << " standard depth 4\n"
        "  " << program_name << " book wdepth 2 bdepth 5 num_games 100 quiet\n"
        "  " << program_name << " book num_games 10\n"
        "\n";
}

// ── Config ────────────────────────────────────────────────────────────────────

struct GameConfig {
    int  white_depth = 1;
    int  black_depth = 4;
    int  num_games   = 1;
    bool use_book    = false;
    bool verbose     = true;
};

GameConfig parse_args(int argc, char* argv[]) {
    GameConfig cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "help")      { print_help(argv[0]); std::exit(0); }
        else if (arg == "book")      cfg.use_book    = true;
        else if (arg == "standard")  cfg.use_book    = false;
        else if (arg == "depth")     cfg.white_depth = cfg.black_depth = std::stoi(argv[++i]);
        else if (arg == "wdepth")    cfg.white_depth = std::stoi(argv[++i]);
        else if (arg == "bdepth")    cfg.black_depth = std::stoi(argv[++i]);
        else if (arg == "num_games") cfg.num_games   = std::stoi(argv[++i]);
        else if (arg == "quiet")     cfg.verbose     = false;
        else {
            std::cerr << "[args] Unknown argument: '" << arg << "'. Run with 'help' for usage.\n";
            std::exit(1);
        }
    }

    return cfg;
}

// ── Result ────────────────────────────────────────────────────────────────────

enum class GameResult { WhiteWins, BlackWins, Draw, Error };

struct GameSummary {
    int white_wins = 0;
    int black_wins = 0;
    int draws      = 0;
    int errors     = 0;

    int total() const { return white_wins + black_wins + draws + errors; }

    void print() const {
        std::cout << "\n=== Results (" << total() << " games) ===\n";
        std::cout << "  White wins : " << white_wins << "\n";
        std::cout << "  Black wins : " << black_wins << "\n";
        std::cout << "  Draws      : " << draws      << "\n";
        if (errors > 0)
            std::cout << "  Errors     : " << errors << "\n";
    }

    void record(GameResult r) {
        switch (r) {
            case GameResult::WhiteWins: white_wins++; break;
            case GameResult::BlackWins: black_wins++; break;
            case GameResult::Draw:      draws++;      break;
            case GameResult::Error:     errors++;     break;
        }
    }
};

// ── Opening book (loaded once) ────────────────────────────────────────────────

PositionBook load_book() {
    PositionBook book = loadEPD("/Users/cahree/Downloads/noob_3moves.epd");
    if (book.empty())
        std::cerr << "[book] Failed to load — will use startpos.\n";
    else
        std::cout << "[book] Loaded " << book.size() << " positions.\n";
    return book;
}

// ── Starting position for one game ───────────────────────────────────────────

BitboardPosition make_start_position(const GameConfig& cfg, const PositionBook& book) {
    if (cfg.use_book && !book.empty()) {
        std::string fen = pickRandomPosition(book);
        std::cout << "[book] Opening: " << fen << "\n";
        // TODO: replace with BitboardPosition::from_fen(fen) once implemented
    }
    return BitboardPosition::startpos();
}

// ── Play one game ─────────────────────────────────────────────────────────────

GameResult play_game(const GameConfig& cfg, const PositionBook& book, int game_number) {
    BitboardPosition pos = make_start_position(cfg, book);

    if (cfg.verbose) {
        std::cout << "\n=== Game " << game_number << " ===\n";
        std::cout << Render::board_ascii(pos) << "\n";
    }

    while (true) {
        bool white_to_move = (pos.side_to_move() == Color::White);
        int  depth         = white_to_move ? cfg.white_depth : cfg.black_depth;
        std::string side   = white_to_move ? "White" : "Black";

        // ── Mate / stalemate ──────────────────────────────────────────────────
        if (!pos.has_any_legal_move()) {
            if (pos.in_check(pos.side_to_move())) {
                std::string winner = white_to_move ? "Black" : "White";
                std::cout << "Checkmate! " << winner << " wins.\n";
                return white_to_move ? GameResult::BlackWins : GameResult::WhiteWins;
            } else {
                std::cout << "Stalemate. Draw.\n";
                return GameResult::Draw;
            }
        }

        // ── Search ───────────────────────────────────────────────────────────
        if (cfg.verbose) std::cout << side << " thinking (depth " << depth << ")...\n";

        SearchResult result = SearchBB::minimax(pos, depth);

        if (cfg.verbose)
            std::cout << side << "'s move: " << Parse::move_to_string(result.best) << "\n\n";

        pos.make_move(result.best);

        if (cfg.verbose)
            std::cout << Render::board_ascii(pos) << "\n";

        // ── Draw checks ───────────────────────────────────────────────────────
        if (pos.is_draw_by_fifty()) {
            std::cout << "Draw by fifty move rule.\n";
            return GameResult::Draw;
        }
        if (pos.is_draw_by_repetition()) {
            std::cout << "Draw by threefold repetition.\n";
            return GameResult::Draw;
        }
    }
}

} // namespace chess

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    using namespace chess;

    GameConfig   cfg     = parse_args(argc, argv);
    PositionBook book    = load_book();
    GameSummary  summary;

    for (int game = 1; game <= cfg.num_games; ++game) {
        GameResult result = play_game(cfg, book, game);
        summary.record(result);
    }

    summary.print();
    return 0;
}