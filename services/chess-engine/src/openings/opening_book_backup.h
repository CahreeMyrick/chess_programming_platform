#pragma once
#include "zobrist.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

// ── Types ─────────────────────────────────────────────────────────────────────

struct BookMove {
    std::string move;   // UCI format e.g. "e2e4"
    int         weight; // for weighted random selection
};

// Zobrist key → candidate moves
using OpeningBook = std::unordered_map<uint64_t, std::vector<BookMove>>;

// ── Declarations ──────────────────────────────────────────────────────────────

// Parse a single EPD line — returns {zobrist_key, move} or nullopt
std::optional<std::pair<uint64_t, std::string>>
parseEPDLine(const std::string& line);

// Load an EPD file into an OpeningBook hash map
OpeningBook loadEPD(const std::string& filepath);

// Probe the book for the given FEN.
// Returns a UCI move string e.g. "e2e4", or "" if not in book.
std::string probeBook(const OpeningBook& book, const std::string& fen);
