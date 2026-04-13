#include "opening_book.h"
#include <iostream>
#include <string>

// ── Stub: replace with your real engine search ────────────────────────────────
std::string engineSearch(const std::string& fen) {
    // Your alpha-beta / minimax search goes here
    return "e2e4"; // placeholder
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main() {
    // 1. Load all positions from the book once at startup
    PositionBook book = loadEPD("/Users/cahree/Downloads/noob_3moves.epd");

    if (book.empty()) {
        std::cerr << "[book] Failed to load. Exiting.\n";
        return 1;
    }

    // 2. Pick a random starting position for this game
    std::string startFEN = pickRandomPosition(book);
    std::cout << "[book] Starting position: " << startFEN << "\n";

    // 3. Hand it straight to your engine — no probing needed
    std::string move = engineSearch(startFEN);
    std::cout << "[engine] Best move: " << move << "\n";

    return 0;
}


