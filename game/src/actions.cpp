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
  
  if (action_id >= DiscardHandBase() && action_id < DiscardHandBase() + MaxHandSize()) {
    return std::string("DiscardHandSelect_") + std::to_string(action_id - DiscardHandBase());
  }
  if (action_id == DiscardHandSelectFinish()) return "DiscardHandSelectFinish";

  if (action_id >= TrashHandBase() && action_id < TrashHandBase() + MaxHandSize()) {
    return std::string("TrashHandSelect_") + std::to_string(action_id - TrashHandBase());
  }
  if (action_id == TrashHandSelectFinish()) return "TrashHandSelectFinish";
  if (action_id == ThroneHandSelectFinish()) return "ThroneHandSelectFinish";
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

  if (action_id == Shuffle()) return "Shuffle";
  if (action_id == ActionIds::DrawNonTerminal()) return "DrawNonTerminal";

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
  } else if (action_id >= DiscardHandBase() && action_id < DiscardHandBase() + num_supply_piles) {
    int j = static_cast<int>(action_id - DiscardHandBase());
    if (j >= 0 && j < num_supply_piles) {
      base += std::string(" (") + cname(static_cast<CardName>(j)) + ")";
    }
  } else if (action_id >= TrashHandBase() && action_id < TrashHandBase() + num_supply_piles) {
    int j = static_cast<int>(action_id - TrashHandBase());
    if (j >= 0 && j < num_supply_piles) {
      base += std::string(" (") + cname(static_cast<CardName>(j)) + ")";
    }
  } else if (action_id >= GainSelectBase() && action_id < GainSelectBase() + num_supply_piles) {
    int j = static_cast<int>(action_id - GainSelectBase());
    if (j >= 0 && j < num_supply_piles) {
      base += std::string(" (") + cname(static_cast<CardName>(j)) + ")";
    }
  }
  return base;
}

} // namespace dominion
} // namespace open_spiel
