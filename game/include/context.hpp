#pragma once
#include <vector>
#include <string>

// GameContext represents the state of any given turn of a game.
class GameContext {
public:
    uint32_t seed; // RNG seed
    int turnCount = 0;
    bool gameEnd = false;
    int actions = 1;
    int buys = 1;
    int money = 0;
    std::vector<std::string> play_area; // placeholder representation

    explicit GameContext(uint32_t seed_ = 0);

    void print_summary() const;
};