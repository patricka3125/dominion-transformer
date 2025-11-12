cmake_minimum_required(VERSION 3.15)
project(dominion_cpp_game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build the core game logic as a static library from this cpp_game/ folder
add_library(dominion_cpp_game STATIC
    src/board_state.cpp
    src/context.cpp
    src/engine.cpp
)

# Public headers live under include/
target_include_directories(dominion_cpp_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Enable common warnings
if (MSVC)
    target_compile_options(dominion_cpp_game PRIVATE /W4)
else()
    target_compile_options(dominion_cpp_game PRIVATE -Wall -Wextra -Wpedantic)
endif()