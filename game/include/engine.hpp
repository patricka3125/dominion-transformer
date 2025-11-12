#pragma once
#include "board_state.hpp"
#include "context.hpp"

// GameEngine is responsible for running the game.
class GameEngine {
public:
    BoardState board_state;
    GameContext game_context;
    size_t current_player_index = 0;

    GameEngine(BoardState bs, GameContext ctx);
    explicit GameEngine(BoardState bs);

    void run();
    void run_turn();
    void enter_action_phase(); // placeholder for future input loops

    Player& currentPlayer();
};