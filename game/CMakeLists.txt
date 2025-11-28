cmake_minimum_required(VERSION 3.15)
project(dominion_cpp_game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configure OpenSpiel location in a portable way.
# Users can set OPEN_SPIEL_ROOT via environment or -DOPEN_SPIEL_ROOT=...
if (NOT DEFINED OPEN_SPIEL_ROOT)
  set(OPEN_SPIEL_ROOT $ENV{OPEN_SPIEL_ROOT})
endif()
find_path(OPEN_SPIEL_INCLUDE_DIR open_spiel/spiel.h
  HINTS
    ${OPEN_SPIEL_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../open_spiel
    ${CMAKE_CURRENT_SOURCE_DIR}/../open_spiel
)
if (NOT OPEN_SPIEL_INCLUDE_DIR)
  message(FATAL_ERROR "OpenSpiel headers not found. Please set OPEN_SPIEL_ROOT to your OpenSpiel clone (e.g., -DOPEN_SPIEL_ROOT=/path/to/open_spiel)")
endif()

# Normalize include dir to the repository root (which contains the open_spiel/ folder).
if (EXISTS "${OPEN_SPIEL_ROOT}/open_spiel/spiel.h")
  set(OPEN_SPIEL_INCLUDE_DIR ${OPEN_SPIEL_ROOT})
elseif (EXISTS "${OPEN_SPIEL_INCLUDE_DIR}/open_spiel/spiel.h")
  # OPEN_SPIEL_INCLUDE_DIR already points to the correct root.
else()
  # As a fallback, check one level up from a nested open_spiel path.
  get_filename_component(_parent "${OPEN_SPIEL_INCLUDE_DIR}" DIRECTORY)
  if (EXISTS "${_parent}/open_spiel/spiel.h")
    set(OPEN_SPIEL_INCLUDE_DIR ${_parent})
  endif()
endif()

# Abseil and JSON headers vendored under OpenSpiel tree.
find_path(ABSL_INCLUDE_DIR absl/base/attributes.h HINTS ${OPEN_SPIEL_INCLUDE_DIR}/open_spiel/abseil-cpp)
find_path(JSON_INCLUDE_DIR nlohmann/json.hpp HINTS ${OPEN_SPIEL_INCLUDE_DIR}/open_spiel/json/include)
if (NOT ABSL_INCLUDE_DIR AND EXISTS "${OPEN_SPIEL_INCLUDE_DIR}/open_spiel/abseil-cpp/absl/base/attributes.h")
  set(ABSL_INCLUDE_DIR ${OPEN_SPIEL_INCLUDE_DIR}/open_spiel/abseil-cpp)
endif()
if (NOT JSON_INCLUDE_DIR AND EXISTS "${OPEN_SPIEL_INCLUDE_DIR}/open_spiel/json/include/nlohmann/json.hpp")
  set(JSON_INCLUDE_DIR ${OPEN_SPIEL_INCLUDE_DIR}/open_spiel/json/include)
endif()

# Build the core game logic as a static library
add_library(dominion_cpp_game STATIC
    src/dominion.cpp
    src/cards.cpp
    src/actions.cpp
    include/effects.hpp
    src/effects.cpp
    src/cards/chapel.cpp
    src/cards/cellar.cpp
    src/cards/workshop.cpp
    src/cards/remodel.cpp
    src/cards/militia.cpp
    src/cards/witch.cpp
    src/cards/throne_room.cpp
)

# Public headers and OpenSpiel headers
target_include_directories(
    dominion_cpp_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPEN_SPIEL_INCLUDE_DIR}
)
if (ABSL_INCLUDE_DIR)
  target_include_directories(dominion_cpp_game PUBLIC ${ABSL_INCLUDE_DIR})
endif()
if (JSON_INCLUDE_DIR)
  target_include_directories(dominion_cpp_game PUBLIC ${JSON_INCLUDE_DIR})
endif()

# Enable common warnings
if (MSVC)
    target_compile_options(dominion_cpp_game PRIVATE /W4)
else()
    target_compile_options(dominion_cpp_game PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Test executable for card effects
find_library(OPEN_SPIEL_LIB open_spiel 
    HINTS 
    ${OPEN_SPIEL_ROOT}/open_spiel/build 
    ${OPEN_SPIEL_ROOT}/build
    ${OPEN_SPIEL_INCLUDE_DIR}/../build 
    ${OPEN_SPIEL_INCLUDE_DIR}/build/open_spiel)
if (OPEN_SPIEL_LIB)
  add_executable(dominion_cards_test
      src/cards_test.cpp
      src/cards/chapel_test.cpp
      src/cards/cellar_test.cpp
      src/cards/workshop_test.cpp
      src/cards/remodel_test.cpp
      src/cards/militia_test.cpp
      src/cards/witch_test.cpp
      src/cards/throne_room_test.cpp
  )
  target_link_libraries(dominion_cards_test
      dominion_cpp_game
      ${OPEN_SPIEL_LIB}
  )
  target_include_directories(dominion_cards_test PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${OPEN_SPIEL_INCLUDE_DIR}
  )
  if (ABSL_INCLUDE_DIR)
    target_include_directories(dominion_cards_test PUBLIC ${ABSL_INCLUDE_DIR})
  endif()
  if (JSON_INCLUDE_DIR)
    target_include_directories(dominion_cards_test PUBLIC ${JSON_INCLUDE_DIR})
  endif()
  add_executable(dominion_test
      src/dominion_test.cpp
  )
  target_link_libraries(dominion_test
      dominion_cpp_game
      ${OPEN_SPIEL_LIB}
  )
  target_include_directories(dominion_test PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${OPEN_SPIEL_INCLUDE_DIR}
  )
  if (ABSL_INCLUDE_DIR)
    target_include_directories(dominion_test PUBLIC ${ABSL_INCLUDE_DIR})
  endif()
  if (JSON_INCLUDE_DIR)
    target_include_directories(dominion_test PUBLIC ${JSON_INCLUDE_DIR})
  endif()
else()
  message(WARNING "OpenSpiel library not found. Provide -DOPEN_SPIEL_ROOT or ensure it is discoverable. Skipping test target.")
endif()
