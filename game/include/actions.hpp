#ifndef OPEN_SPIEL_GAMES_DOMINION_ACTIONS_H_
#define OPEN_SPIEL_GAMES_DOMINION_ACTIONS_H_

#include <string>
#include <vector>
#include "cards.hpp"

#include "dominion.hpp"
#include "open_spiel/spiel.h"

namespace open_spiel {
namespace dominion {

// Centralized action ID registry. Avoids magic numbers by providing
// clearly named constructors and queries for action IDs used by Dominion.
namespace ActionIds {
  // Maximum possible hand size cap used for indexing/select ranges.
  inline int MaxHandSize() { return kNumSupplyPiles; }
  // Maximum supply piles equals number of CardName enumerators.
  inline int MaxSupplyPiles() { return kNumSupplyPiles; }
  // Hand play is indexed directly by hand position during phases.
  // PlayHandIndex maps directly to CardName enumerator id present in hand counts.
  inline Action PlayHandIndex(int i) { return static_cast<Action>(i); }
  
  // Discard selection actions (effect-level, e.g., Cellar, Militia).
  inline int DiscardHandBase() { return MaxHandSize(); }
  inline Action DiscardHandSelect(int i) { return static_cast<Action>(DiscardHandBase() + i); }
  inline Action DiscardHandSelectFinish() { return static_cast<Action>(DiscardHandBase() + MaxHandSize()); }

  // Trash selection actions (effect-level, e.g., Chapel, Remodel).
  inline int TrashHandBase() { return DiscardHandBase() + MaxHandSize() + 1; }
  inline Action TrashHandSelect(int i) { return static_cast<Action>(TrashHandBase() + i); }
  inline Action TrashHandSelectFinish() { return static_cast<Action>(TrashHandBase() + MaxHandSize()); }
  
  // Throne Room selection finish (effect-level): ends the current throne selection.
  inline Action ThroneHandSelectFinish() { return static_cast<Action>(TrashHandSelectFinish() + 1); }
  
  // Phase control actions.
  inline Action EndActions() { return static_cast<Action>(ThroneHandSelectFinish()+1); }

  // Buying from supply uses a base offset plus supply pile index.
  inline int BuyBase() { return EndActions()+1; }
  inline Action BuyFromSupply(int j) { return static_cast<Action>(BuyBase() + j); }
  inline Action EndBuy() { return static_cast<Action>(BuyBase()+MaxSupplyPiles()); }


  // Generic gain-from-supply selection actions (effect-level).
  inline int GainSelectBase() { return EndBuy()+1; }
  inline Action GainSelect(int j) { return static_cast<Action>(GainSelectBase() + j); }

  // Chance outcome used in sampled stochastic mode for deck shuffling.
  inline Action Shuffle() { return GainSelectBase() + kNumSupplyPiles; }

  // Composite heuristic action: play a non-terminal action chosen by engine.
  inline Action PlayNonTerminal() { return static_cast<Action>(Shuffle() + 1); }
}

// Human-readable names for action IDs. The caller provides the supply size
// to disambiguate buy actions.
namespace ActionNames {
  std::string Name(Action action_id, int num_supply_piles);
  std::string NameWithCard(Action action_id,
                           int num_supply_piles);
}

} // namespace dominion
} // namespace open_spiel

#endif
