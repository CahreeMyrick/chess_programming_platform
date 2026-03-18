#pragma once
#include <string>
#include <optional>
#include "types.hpp"

namespace chess {

class Parse {
public:
    // "e2" -> square index (0=a1 .. 63=h8). returns nullopt if invalid.
    static std::optional<int> square(const std::string& s);

    // "e2 e4" -> {from,to}. Ignores leading/trailing spaces.
    // returns nullopt if invalid format.
    static std::optional<std::pair<int,int>> two_squares(const std::string& line);
};

} // namespace chess

