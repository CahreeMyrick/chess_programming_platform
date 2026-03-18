#include "position/piece.hpp"

#include <memory>

namespace chess::pointerboard {

std::unique_ptr<PieceObject> make_piece_object(chess::Piece p) {
    switch (p) {
        case chess::Piece::WP:
            return std::make_unique<Pawn>(Color::White);
        case chess::Piece::BP:
            return std::make_unique<Pawn>(Color::Black);

        case chess::Piece::WN:
            return std::make_unique<Knight>(Color::White);
        case chess::Piece::BN:
            return std::make_unique<Knight>(Color::Black);

        case chess::Piece::WB:
            return std::make_unique<Bishop>(Color::White);
        case chess::Piece::BB:
            return std::make_unique<Bishop>(Color::Black);

        case chess::Piece::WR:
            return std::make_unique<Rook>(Color::White);
        case chess::Piece::BR:
            return std::make_unique<Rook>(Color::Black);

        case chess::Piece::WQ:
            return std::make_unique<Queen>(Color::White);
        case chess::Piece::BQ:
            return std::make_unique<Queen>(Color::Black);

        case chess::Piece::WK:
            return std::make_unique<King>(Color::White);
        case chess::Piece::BK:
            return std::make_unique<King>(Color::Black);

        case chess::Piece::Empty:
        default:
            return nullptr;
    }
}

} // namespace chess::pointerboard