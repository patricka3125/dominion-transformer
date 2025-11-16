#include "actions.hpp"

namespace open_spiel {
namespace dominion {

// Maps an action_id to a readable string label.
// This improves readability across large switch-like functions.
std::string ActionNames::Name(Action action_id, int num_supply_piles) {
  using namespace ActionIds;
  if (action_id < BuyBase()) {
    return std::string("PlayHandIndex_") + std::to_string(action_id);
  }
  if (action_id >= BuyBase() && action_id < BuyBase() + num_supply_piles) {
    int j = static_cast<int>(action_id - BuyBase());
    return std::string("Buy_") + std::to_string(j);
  }
  if (action_id == EndActions()) return "EndActions";
  if (action_id == EndBuy()) return "EndBuy";

  // Finish must be recognized before the generic select range, since 399
  // overlaps the select range upper bound.
  if (action_id == HandSelectFinish()) return "HandSelectFinish";
  if (action_id >= HandSelectBase() && action_id < HandSelectBase() + 100) {
    return std::string("HandSelect_") + std::to_string(action_id - HandSelectBase());
  }

  if (action_id >= GainSelectBase() && action_id < GainSelectBase() + num_supply_piles) {
    int j = static_cast<int>(action_id - GainSelectBase());
    return std::string("GainSelect_") + std::to_string(j);
  }

  return std::string("Action_") + std::to_string(action_id);
}

} // namespace dominion
} // namespace open_spiel