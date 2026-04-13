#include "opening_book.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>

// ── EPD Parser ────────────────────────────────────────────────────────────────

std::optional<std::pair<uint64_t, std::string>>
parseEPDLine(const std::string& line) {
    if (line.empty() || line[0] == '#') return std::nullopt;

    std::istringstream ss(line);
    std::vector<std::string> tokens;
    std::string part;
    while (ss >> part)
        tokens.push_back(part);

    // EPD needs at least 4 fields: placement, side, castling, en passant
    if (tokens.size() < 4) return std::nullopt;

    // Reconstruct FEN (EPD omits move counters — add defaults)
    std::string fen = tokens[0] + " " + tokens[1] + " " +
                      tokens[2] + " " + tokens[3] + " 0 1";

    // Look for "bm" (best move) annotation
    std::string move;
    for (size_t i = 4; i + 1 < tokens.size(); i++) {
        if (tokens[i] == "bm") {
            move = tokens[i + 1];
            // Strip trailing semicolon if present
            if (!move.empty() && move.back() == ';')
                move.pop_back();
            break;
        }
    }

    uint64_t key = zobristFromFEN(fen);
    return std::make_pair(key, move);
}

// ── Loader ────────────────────────────────────────────────────────────────────

OpeningBook loadEPD(const std::string& filepath) {
    OpeningBook book;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "[book] ERROR: Could not open " << filepath << "\n";
        return book;
    }

    std::string line;
    int total = 0, loaded = 0;

    while (std::getline(file, line)) {
        std::cout << "Parsing line: " << line << "\n";
        total++;
        auto result = parseEPDLine(line);
        // std::cout << "Parsed result: " << (result ? "valid" : "invalid") << "\n";
        if (!result) continue;

        auto [key, move] = *result;
        std::cout << "Zobrist key: " << key << ", move: " << move << "\n";
        if (!move.empty()) {
            book[key].push_back({move, 1});
            loaded++;
        }
    }

    std::cout << "[book] Loaded " << loaded << " / " << total
              << " positions from " << filepath << "\n";
    return book;
}

// ── Probe ─────────────────────────────────────────────────────────────────────

std::string probeBook(const OpeningBook& book, const std::string& fen) {
    uint64_t key = zobristFromFEN(fen);

    auto it = book.find(key);
    if (it == book.end())
        return ""; // not in book — fall through to search

    const auto& moves = it->second;

    // Weighted random selection
    static std::mt19937 rng(std::random_device{}());
    int totalWeight = 0;
    for (const auto& m : moves) totalWeight += m.weight;

    std::uniform_int_distribution<int> dist(0, totalWeight - 1);
    int roll = dist(rng);

    for (const auto& m : moves) {
        roll -= m.weight;
        if (roll < 0) return m.move;
    }

    return moves.back().move;
}
