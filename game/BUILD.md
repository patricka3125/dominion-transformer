# Dominion Transformer — Build Instructions

This project integrates with OpenSpiel and builds a static library plus a small test binary. The build is reproducible for any user by providing the path to an OpenSpiel clone.

## Prerequisites
- CMake 3.15+
- A C++17 compiler (AppleClang/Clang/GCC)
- OpenSpiel cloned locally

## 1) Build OpenSpiel
Set `OPEN_SPIEL_ROOT` to the path containing your `open_spiel` clone, then build OpenSpiel:

```bash
export OPEN_SPIEL_ROOT=/absolute/path/to/open_spiel
cmake -S "$OPEN_SPIEL_ROOT/open_spiel" -B "$OPEN_SPIEL_ROOT/open_spiel/build"
cmake --build "$OPEN_SPIEL_ROOT/open_spiel/build" -j 8
```

This produces the `open_spiel` library in the OpenSpiel build directory.

## 2) Build Dominion (this repo)
Run CMake for the `game/` directory, passing `OPEN_SPIEL_ROOT` so headers and libraries can be discovered:

```bash
cd dominion-transformer/game
cmake -S . -B build -DOPEN_SPIEL_ROOT="$OPEN_SPIEL_ROOT"
cmake --build build -j 8
```

Alternatively, an in-source build is supported:

```bash
cmake -S . -B . -DOPEN_SPIEL_ROOT="$OPEN_SPIEL_ROOT"
cmake --build . -j 8
```

## 3) Run Tests
The test target (`dominion_cards_test`) is generated only if the `open_spiel` library is found.

```bash
./build/dominion_cards_test
# or if you built in-source:
./dominion_cards_test
```

## Notes
- Includes are resolved from the OpenSpiel tree (`open_spiel/…`) and its vendored Abseil (`open_spiel/abseil-cpp`) and nlohmann JSON (`open_spiel/json/include`).
- If you see "OpenSpiel headers not found", set `OPEN_SPIEL_ROOT` or pass it via `-DOPEN_SPIEL_ROOT=…` to CMake.
- If Abseil/JSON headers are not found, ensure your `OPEN_SPIEL_ROOT` points to the top-level OpenSpiel directory (the one that contains `open_spiel/`).