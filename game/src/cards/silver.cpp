#include "cards.hpp"
#include "dominion.hpp"

namespace open_spiel {
namespace dominion {

void SilverCard::applyEffect(DominionState& state, int player) const {
  if (!state.first_silver_played_this_turn_) {
    int bonus = 0;
    for (auto cn : state.play_area_) {
      if (cn == CardName::CARD_Merchant) bonus += 1;
    }
    state.coins_ += bonus;
    state.first_silver_played_this_turn_ = true;
  }
}

}  // namespace dominion
}  // namespace open_spiel

