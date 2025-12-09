#include "cards.hpp"
#include "dominion.hpp"

namespace open_spiel {
namespace dominion {

// Moneylender: automatically trash one Copper from hand if present, then grant +3 coins.
void MoneylenderCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  int copper_idx = static_cast<int>(CardName::CARD_Copper);
  if (copper_idx >= 0 && copper_idx < kNumSupplyPiles && ps.hand_counts_[copper_idx] > 0) {
    ps.hand_counts_[copper_idx] -= 1;
    state.coins_ += 3;
  }
}

}  // namespace dominion
}  // namespace open_spiel

