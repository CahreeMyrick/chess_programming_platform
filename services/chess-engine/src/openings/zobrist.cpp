#include "zobrist.h"
#include <sstream>

// ── Global instance ───────────────────────────────────────────────────────────
ZobristTable ZOBRIST;

// ── Constructor ───────────────────────────────────────────────────────────────
ZobristTable::ZobristTable() {
    // Fixed seed = reproducible keys across runs
    uint64_t s = 1070372ULL;
    auto rand64 = [&]() {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        return s;
    };

    for (auto& sq : pieces)
        for (auto& p : sq)
            p = rand64();

    sideToMove = rand64();

    for (auto& c : castling)  c = rand64();
    for (auto& e : enPassant) e = rand64();
}

// ── zobristFromFEN ────────────────────────────────────────────────────────────
// FEN example: "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"
uint64_t zobristFromFEN(const std::string& fen) {
    std::istringstream ss(fen);
    std::string placement, side, castling, enpassant;
    ss >> placement >> side >> castling >> enpassant;

    uint64_t key = 0;

    // --- Piece placement ---
    int sq = 56; // start at a8 (top-left in FEN)
    for (char c : placement) {
        if (c == '/') {
            sq -= 16; // move down a rank
        } else if (c >= '1' && c <= '8') {
            sq += (c - '0');
        } else {
            Piece p = NO_PIECE;
            switch (c) {
                case 'P': p = W_PAWN;   break; case 'N': p = W_KNIGHT; break;
                case 'B': p = W_BISHOP; break; case 'R': p = W_ROOK;   break;
                case 'Q': p = W_QUEEN;  break; case 'K': p = W_KING;   break;
                case 'p': p = B_PAWN;   break; case 'n': p = B_KNIGHT; break;
                case 'b': p = B_BISHOP; break; case 'r': p = B_ROOK;   break;
                case 'q': p = B_QUEEN;  break; case 'k': p = B_KING;   break;
            }
            if (p != NO_PIECE)
                key ^= ZOBRIST.pieces[sq][p];
            sq++;
        }
    }

    // --- Side to move ---
    if (side == "b")
        key ^= ZOBRIST.sideToMove;

    // --- Castling rights ---
    int castlingIndex = 0;
    for (char c : castling) {
        switch (c) {
            case 'K': castlingIndex |= 1; break;
            case 'Q': castlingIndex |= 2; break;
            case 'k': castlingIndex |= 4; break;
            case 'q': castlingIndex |= 8; break;
        }
    }
    key ^= ZOBRIST.castling[castlingIndex];

    // --- En passant ---
    if (enpassant != "-") {
        int file = enpassant[0] - 'a'; // 0-7
        key ^= ZOBRIST.enPassant[file];
    }

    return key;
}
