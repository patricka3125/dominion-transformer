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
* Represent cards and card effects as fixed feature embeddings + card effect embeddings.
* Need to have json serializability for effect nodes in player effect queue.
* Need to implement refining the data that is returned by Information and Observation state.
* Design observation state with MLP NN architecture in mind.

* Need to implement cycle decay when a player passes turn without doing anything (end action -> end buy). One idea is to maintain a global visited state set. If a player finishes a turn and the game state is the same as when it started, then a penalty should be added to their utility.
    * A penalty should be added if an action is selected that is unnecessary. e.g. play 1 copper, buy a curse. (when they could've just bought the curse without playing the copper).
    * (optional)Set games that exceed 30 turns to terminal, or a sever utility punishment.
    * Idea to model the game to be deterministically stochastic. I think it is possible if we make the chance node expansion to C(deckSize, drawCount) and calculate probability for each.
