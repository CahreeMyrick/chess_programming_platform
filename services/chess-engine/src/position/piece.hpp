#pragma once

#include <memory>
#include "core/types.hpp"

namespace chess::pointerboard {

class PieceObject {
public:
    virtual ~PieceObject() = default;

    virtual Color color() const = 0;
    virtual char display() const = 0;
    virtual chess::Piece code() const = 0;
};

class Pawn : public PieceObject {
public:
    explicit Pawn(Color c) : color_(c) {}

    Color color() const override { return color_; }
    char display() const override { return color_ == Color::White ? 'P' : 'p'; }
    chess::Piece code() const override {
        return color_ == Color::White ? chess::Piece::WP : chess::Piece::BP;
    }

private:
    Color color_;
};

class Knight : public PieceObject {
public:
    explicit Knight(Color c) : color_(c) {}

    Color color() const override { return color_; }
    char display() const override { return color_ == Color::White ? 'N' : 'n'; }
    chess::Piece code() const override {
        return color_ == Color::White ? chess::Piece::WN : chess::Piece::BN;
    }

private:
    Color color_;
};

class Bishop : public PieceObject {
public:
    explicit Bishop(Color c) : color_(c) {}

    Color color() const override { return color_; }
    char display() const override { return color_ == Color::White ? 'B' : 'b'; }
    chess::Piece code() const override {
        return color_ == Color::White ? chess::Piece::WB : chess::Piece::BB;
    }

private:
    Color color_;
};

class Rook : public PieceObject {
public:
    explicit Rook(Color c) : color_(c) {}

    Color color() const override { return color_; }
    char display() const override { return color_ == Color::White ? 'R' : 'r'; }
    chess::Piece code() const override {
        return color_ == Color::White ? chess::Piece::WR : chess::Piece::BR;
    }

private:
    Color color_;
};

class Queen : public PieceObject {
public:
    explicit Queen(Color c) : color_(c) {}

    Color color() const override { return color_; }
    char display() const override { return color_ == Color::White ? 'Q' : 'q'; }
    chess::Piece code() const override {
        return color_ == Color::White ? chess::Piece::WQ : chess::Piece::BQ;
    }

private:
    Color color_;
};

class King : public PieceObject {
public:
    explicit King(Color c) : color_(c) {}

    Color color() const override { return color_; }
    char display() const override { return color_ == Color::White ? 'K' : 'k'; }
    chess::Piece code() const override {
        return color_ == Color::White ? chess::Piece::WK : chess::Piece::BK;
    }

private:
    Color color_;
};

std::unique_ptr<PieceObject> make_piece_object(chess::Piece p);

} // namespace chess::pointerboard