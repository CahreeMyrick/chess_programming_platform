#include "openings/opening_book.h"
#include <iostream>
#include <string>

// ── Stub: replace this with your real engine search ──────────────────────────
std::string engineSearch(const std::string& fen) {
    // Your alpha-beta / minimax search goes here
    return "e2e4"; // placeholder
}

// ── Main game loop ────────────────────────────────────────────────────────────
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


    return 0;
}

// ── How to integrate into your engine ────────────────────────────────────────
//
// In your engine's move selection function, replace whatever you currently
// have with this pattern:
//
//   std::string getBestMove(const std::string& fen) {
//       std::string bookMove = probeBook(book, fen);
//       if (!bookMove.empty()) return bookMove;   // use book move
//       return engineSearch(fen);                 // fall back to search
//   }
//
// The book variable should be loaded once at startup (in main or engine init)
// and passed around — or stored as a global/singleton.
//
// ── How to compile ───────────────────────────────────────────────────────────
//
//   g++ -std=c++17 -O2 -o engine main.cpp
//
// All three files must be in the same directory:
//   main.cpp
//   opening_book.h
//   zobrist.h
