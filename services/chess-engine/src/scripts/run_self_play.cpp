#include <iostream>
#include <string>

#include "selfplay/selfplay.hpp"
#include "selfplay/match.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  ./chess_engine data\n";
        std::cout << "  ./chess_engine match\n";
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "data") {
        chess::run_selfplay(
            /*num_games=*/10000,
            /*depth=*/4,
            "selfplay.csv"
        );
    }
    else if (mode == "match") {
        chess::run_engine_match(
            /*games_per_side=*/20,
            /*depth=*/5,
            /*max_plies=*/300
        );
    }
    else {
        std::cerr << "Unknown mode: " << mode << "\n";
        return 1;
    }

    return 0;
}