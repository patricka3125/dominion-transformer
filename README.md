# dominion-transformer

An implementation of the popular deck builder, compatible with OpenSpiel framework. 

## TODO: 
* Need to implement cycle decay when a player passes turn without doing anything (end action -> end buy). One idea is to maintain a global visited state set. If a player finishes a turn and the game state is the same as when it started, then a penalty should be added to their utility.
* Need to implement refining the data that is returned by Information and Observation state
* Need to implement supporting logic for chance nodes for kSampleStochasity
