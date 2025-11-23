#include "actions.hpp"
#include "cards.hpp"

namespace open_spiel {
namespace dominion {

// Maps an action_id to a readable string label.
// This improves readability across large switch-like functions.
std::string ActionNames::Name(Action action_id, int num_supply_piles) {
  using namespace ActionIds;
  if (action_id < MaxHandSize()) {
    return std::string("PlayHandIndex_") + std::to_string(action_id);
  }
  
  if (action_id >= HandSelectBase() && action_id < HandSelectBase() + MaxHandSize()) {
    return std::string("HandSelect_") + std::to_string(action_id - HandSelectBase());
  }
  if (action_id == HandSelectFinish()) return "HandSelectFinish";
  if (action_id == EndActions()) return "EndActions";
  if (action_id == EndBuy()) return "EndBuy";

  if (action_id >= BuyBase() && action_id < BuyBase() + num_supply_piles) {
    int j = static_cast<int>(action_id - BuyBase());
    return std::string("Buy_") + std::to_string(j);
  }

  if (action_id >= GainSelectBase() && action_id < GainSelectBase() + num_supply_piles) {
    int j = static_cast<int>(action_id - GainSelectBase());
    return std::string("GainSelect_") + std::to_string(j);
  }

  return std::string("Unknown_") + std::to_string(action_id);
}

// Context-rich name that annotates play/buy actions with the concrete card.
std::string ActionNames::NameWithCard(Action action_id,
                                      int num_supply_piles) {
  using namespace ActionIds;
  auto base = Name(action_id, num_supply_piles);
  auto cname = [](CardName cn) { return GetCardSpec(cn).name_; };
  if (action_id < MaxHandSize()) {
    int idx = static_cast<int>(action_id);
    if (idx >= 0 && idx < kNumSupplyPiles) {
      base += std::string(" (") + cname(static_cast<CardName>(idx)) + ")";
    }
  } else if (action_id >= BuyBase() && action_id < BuyBase() + num_supply_piles) {
    int j = static_cast<int>(action_id - BuyBase());
    if (j >= 0 && j < num_supply_piles) {
      base += std::string(" (") + cname(static_cast<CardName>(j)) + ")";
    }
  }
  return base;
}

} // namespace dominion
} // namespace open_spiel