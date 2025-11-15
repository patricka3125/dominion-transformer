cmake_minimum_required(VERSION 3.15)
project(dominion_cpp_game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Path to your OpenSpiel clone
set(OPEN_SPIEL_DIR "/Users/bytedance/dominion/open_spiel/")

# Build the core game logic as a static library
add_library(dominion_cpp_game STATIC
    src/dominion.cpp
    src/cards.cpp
    src/actions.cpp
)

# Public headers and OpenSpiel headers
target_include_directories(
    dominion_cpp_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPEN_SPIEL_DIR}
    ${OPEN_SPIEL_DIR}/open_spiel/abseil-cpp
    ${OPEN_SPIEL_DIR}/open_spiel/json/include
)

# Enable common warnings
if (MSVC)
    target_compile_options(dominion_cpp_game PRIVATE /W4)
else()
    target_compile_options(dominion_cpp_game PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Test executable for card effects
find_library(OPEN_SPIEL_LIB open_spiel 
    PATHS 
    ${OPEN_SPIEL_DIR}/open_spiel/build 
    ${OPEN_SPIEL_DIR}/open_spiel/build/open_spiel)
if (OPEN_SPIEL_LIB)
  add_executable(dominion_cards_test
      src/cards_test.cpp
  )
  target_link_libraries(dominion_cards_test
      dominion_cpp_game
      ${OPEN_SPIEL_LIB}
  )
  target_include_directories(dominion_cards_test PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${OPEN_SPIEL_DIR}
      ${OPEN_SPIEL_DIR}/open_spiel/abseil-cpp
      ${OPEN_SPIEL_DIR}/open_spiel/json/include
  )
else()
  message(WARNING "OpenSpiel library not found in ${OPEN_SPIEL_DIR}/open_spiel/build. Skipping test target.")
endif()