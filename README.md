# dominion-transformer

An implementation of the popular deck builder, compatible with OpenSpiel framework. 

## Test and Build instructions:

Test:
```shell
cmake --build /path/to/dominion-transformer/game/build -j 8 && ./dominion-transformer/game/build/dominion_test && ./dominion-transformer/game/build/dominion_cards_test
```

Build and generate playthrough:
```shell
bash -lc 'cmake --build /path/to/open_spiel/build -j 8 && source /path/to/open_spiel/venv/bin/activate && cd /path/to/open_spiel && open_spiel/scripts/generate_new_playthrough.sh dominion'
```

## TODO (in order): 
* Need to implement refining the data that is returned by Information and Observation state
    * wip prompt: Hmm I think the scope is actually a bit too wide. Let's evaluate how tic tac toe defines TicTacToeStructContents, and how TicTacToeStateStruct and TicTacToeObservationStruct extens to this as well as its open_spiel parents (e.g. statestruct, observationstruct). 

Dominion should have a similar structure, except that Dominion is an imperfect information game, so the observation struct should contain partial information of the state struct. 

To keep the codebase organized, let's define all struct contents in a new file.

* Need to implement cycle decay when a player passes turn without doing anything (end action -> end buy). One idea is to maintain a global visited state set. If a player finishes a turn and the game state is the same as when it started, then a penalty should be added to their utility.
    * A penalty should be added if an action is selected that is unnecessary. e.g. play 1 copper, buy a curse. (when they could've just bought the curse without playing the copper).
    * (optional)Set games that exceed 30 turns to terminal, or a sever utility punishment.
    * Idea to model the game to be deterministically stochastic. I think it is possible if we make the chance node expansion to C(deckSize, drawCount) and calculate probability for each.

## Note:
Alphazero + MCTS is still not able to play dominion well! Alphazero is a Neural Net (NN) that contains a policy head and value head given the true state of the game. During actual play, you will never get the true state, so the next best thing will be to feed the observation tensor.

The flaw of observation tensor is that the same observation tensor can actually apply to `n` quantity of deterministic hidden states, which theoretically each have their own policy and value heads. Sure, the NN will be able to aggregate the policy and value heads over all its sampled hidden states, but this will still not be accurate. 

Ideally alphazero is still running alongside MCTS even during an actual game, so that it can perform lookahead on the current gamestate to refine the policy and value head. However this simply isn't possible in an imperfect information game. Or at the very least, the MCTS algorithm that is running needs to be able to continuously sample the hidden states of the game and aggregate policy and value heads based on the results.

In conclusion, we need to have AlphaZero run alongside IS-MCTS, that will produce the best outcome because IS-MCTS will be able to sample n hidden states given an information set, and aggregate the policy and value heads based on the results.
