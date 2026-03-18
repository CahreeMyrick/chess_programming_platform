#include <iostream>
#include "core/types.hpp"

int main() {
    using namespace chess;

    Piece p = Piece::WK;
    std::cout << static_cast<int>(p) << std::endl;
    return 0;
}
