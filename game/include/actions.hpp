#ifndef OPEN_SPIEL_GAMES_DOMINION_ACTIONS_H_
#define OPEN_SPIEL_GAMES_DOMINION_ACTIONS_H_

#include <string>
#include <vector>
#include "cards.hpp"

#include "open_spiel/spiel.h"

namespace open_spiel {
namespace dominion {

// Centralized action ID registry. Avoids magic numbers by providing
// clearly named constructors and queries for action IDs used by Dominion.
namespace ActionIds {
  // Hand play is indexed directly by hand position during phases.
  inline Action PlayHandIndex(int i) { return static_cast<Action>(i); }

  // Buying from supply uses a base offset plus supply pile index.
  inline int BuyBase() { return 100; }
  inline Action BuyFromSupply(int j) { return static_cast<Action>(BuyBase() + j); }

  // Phase control actions.
  inline Action EndActions() { return static_cast<Action>(200); }
  inline Action EndBuy() { return static_cast<Action>(201); }

  // Generic hand selection effect actions.
  inline int HandSelectBase() { return 300; }
  inline Action HandSelect(int i) { return static_cast<Action>(HandSelectBase() + i); }
  inline Action HandSelectFinish() { return static_cast<Action>(399); }

  // Generic gain-from-supply selection actions (effect-level).
  inline int GainSelectBase() { return 500; }
  inline Action GainSelect(int j) { return static_cast<Action>(GainSelectBase() + j); }
}

// Human-readable names for action IDs. The caller provides the supply size
// to disambiguate buy actions.
namespace ActionNames {
  std::string Name(Action action_id, int num_supply_piles);
  std::string NameWithCard(Action action_id,
                           int num_supply_piles,
                           const std::vector<CardName>& hand,
                           const CardName* supply_types);
}

} // namespace dominion
} // namespace open_spiel

#endif
