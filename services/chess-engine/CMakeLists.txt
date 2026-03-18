cmake_minimum_required(VERSION 3.16)
project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(chess_engine
    src/scripts/main.cpp
    src/position/array_board.cpp
    src/position/position.cpp
    src/position/pointer_board.cpp
    src/position/piece.cpp
    src/render/render.cpp
)

target_include_directories(chess_engine PRIVATE src)
