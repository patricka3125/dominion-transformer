#include "../include/context.hpp"
#include <random>
#include <iostream>

GameContext::GameContext(uint32_t seed_) {
    if (seed_ == 0) {
        std::random_device rd;
        seed = rd();
    } else {
        seed = seed_;
    }
}

void GameContext::print_summary() const {
    std::cout << "Game status: game end:" << (gameEnd ? "true" : "false")
              << ", turnCount=" << turnCount << std::endl;
}