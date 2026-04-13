#pragma once
#include <array>
#include <cstdint>
#include <string>

// Piece indices: 0-5 = white P,N,B,R,Q,K | 6-11 = black P,N,B,R,Q,K
enum Piece {
    W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    NO_PIECE
};

struct ZobristTable {
    std::array<std::array<uint64_t, 12>, 64> pieces;
    uint64_t                                  sideToMove;
    std::array<uint64_t, 16>                  castling;
    std::array<uint64_t, 8>                   enPassant;

    ZobristTable();
};

// Global table — defined once in zobrist.cpp
extern ZobristTable ZOBRIST;

// Compute a Zobrist key from a FEN string
uint64_t zobristFromFEN(const std::string& fen);
