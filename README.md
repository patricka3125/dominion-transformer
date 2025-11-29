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

## Known bugs:
* Remodel should not present finish option during trashing, and the effect should resolve if there are no cards left to be trashed in hand.
* Automatically play action if there is only one legal action.

## TODO (in order):
* DrawNonTerminal macro action should stop drawing if there is overdraw from deck (even if cards exist in discard). Currently it will automatically draw from discard.
* Convert DrawNonTerminal to PlayNonTerminal and allow automatic card playing for non-terminal cards that may not produce draw.
* Add macro actions for buying cards.
* Represent cards and card effects as fixed feature embeddings + card effect embeddings.
* Design observation tensor with MLP NN architecture in mind.
* Exclude draw cards that install effect nodes on applyEffect from non-terminal candidates for DrawNonTerminal (requires a per-card flag to mark effect-node-producing draws).

### Training Optimization ideas:
#### Improvements to training efficiency and cost.
* Add macro actions for buying. Instead of play copper, we can just have only the buy actions laid out.
* Decrease simulations for decisions that have low policy head entropy and also have lower noise weighting.
    * See https://claude.ai/share/d1f122df-ce87-4e5d-b353-b6071d8c72df

#### Improvements to accuracy and learning progression.
* Start by training on bots with known strategy (e.g. big money). I can also do this on a money board to make sure it doesn't overfit big money strategy for engine boards.

#### Optimization on NN Warm-start
* Use Neo4J graph DB to store trained data from replay buffer. We can encode the latent NN state pre head split to get an embedding equal to the size of the hidden units of the MLP model. This way we can retrieve replay data of games with similar latent states and use it to warm start the Neural Network. This can potentially net us with the option to reuse policy heads more aggresively and even decrease simulation count to get good results. **NOTE that we need to be careful and try to eliminate states that were trained before NN reaches maturity, since there will be embedding drift.** One idea is to track ELO rating to determine whether a state is usable. Would have to put a match between old model and new model. 
    * Another use case is to create common masking patterns for kingdom embeddings. We can then extract a sample of the opening data from root nodes that have this kingdom to determine a very accurate opening policy and evaluation. 
