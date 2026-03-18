#include <iostream>
#include <memory>

#include "position/array_board.hpp"
#include "position/position.hpp"
#include "render/render.hpp"

int main() {
    chess::Position pos = chess::Position::startpos(
        std::make_unique<chess::ArrayBoard>()
    );

    std::cout << chess::Render::board_ascii(pos);

    return 0;
}