#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

// Witch attack effect: each opponent gains a Curse to their discard if available.
void WitchCard::WitchAttackGiveCurse(DominionState& st, int player) {
  int opp = 1 - player;
  int curse_idx = static_cast<int>(CardName::CARD_Curse);
  SPIEL_CHECK_TRUE(curse_idx >= 0 && curse_idx < kNumSupplyPiles);
  if (st.supply_piles_[curse_idx] > 0) {
    st.player_states_[opp].discard_counts_[curse_idx] += 1;
    st.supply_piles_[curse_idx] -= 1;
  }
}

void WitchCard::applyEffect(DominionState& state, int player) const {
  WitchCard::WitchAttackGiveCurse(state, player);
}

}  // namespace dominion
}  // namespace open_spiel

