#include "selfplay/selfplay.hpp"

int main() {
    chess::run_selfplay(/*num_games=*/50, /*depth=*/4, "selfplay.csv");
}