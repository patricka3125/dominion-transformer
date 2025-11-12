#include "../include/engine.hpp"

GameEngine::GameEngine(BoardState bs, GameContext ctx)
    : board_state(std::move(bs)), game_context(std::move(ctx)) {}

GameEngine::GameEngine(BoardState bs)
    : board_state(std::move(bs)), game_context(GameContext()) {}

Player& GameEngine::currentPlayer() {
    return board_state.players[current_player_index % board_state.players.size()];
}

void GameEngine::run() {
    // Setup the game state.
    for (auto& player : board_state.players) {
        player.shuffle_deck(game_context);
        player.draw(game_context, 5);
    }

    // Start the game loop.
    while (!game_context.gameEnd) {
        run_turn();
    }
}

void GameEngine::run_turn() {
    game_context.turnCount += 1;
    game_context.actions = 1;
    game_context.buys = 1;
    game_context.money = 0;

    // Resolve any active conditions before entering action phase.
    currentPlayer().resolve_conditions(game_context);

    // Placeholder: without an input loop, end after one turn to avoid infinite loop.
    game_context.gameEnd = true;
}

/* 
    The input loop should take the game context as a parameter. 
    1. Based on the game context, the input loop should determine the list of possible actions 
    and prompt the player to select an action.
        1.1. A dictionary of callback functions should be returned by the input loop with all available 
        options as keys. The callback function signature should be func(game_context: GameContext) -> None.
        Each type of action should be defined in effects.py and mapped 1:1 to effects.EffectCategory.
    2. Once an action is selected, the input_loop will process the action, update the game context, and
    prompt the player again should another action be available.
    3. If a processed action results in self.game_context.actions = 0, enter_action_phase will finish.

    * An available action is "End phase". This will automatically set self.game_context.actions = 0.
    * There should be an input loop defined for action and buy phase.
    * In the future, there may need to be special input loops defined for special cards (e.g. Donate,
    resolving conditions), so it is better to make the input loop logic reusable where applicable.
    * Input loops should support a recursive structure. (i.e. input loop A calls input loop B)
    * If the input loop is at the root level (e.g. player is selecting a card to play), game context
    should be updated in place. If the input loop is nested, a new copy of game context should be 
    created and passed to the next input loop, and the returned game context can be applied to the 
    game engine context. This way we can save each play as a unit but do not have to worry about preserving
    complex state changes.
    * Each player should have their own channel listening to input loop prompts. The implementation of this
    channel should be implementing an interface.

    An example input loop chain:
    1. Play card prompt: return a callback function with a list of available cards.
    2. Once a card is played, the card effect should be applied and determine if input loop should continue
    or end.
    3. Opponent input loop: Certain card effects require action from the opponent (e.g. Militia). A new input
    loop should be triggered for the opponent to take action.
*/

void GameEngine::enter_action_phase() {
    // TODO: Input loops should be implemented here, following the Python design notes.
}