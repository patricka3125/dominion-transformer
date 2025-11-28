#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

void WorkshopCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  {
    std::unique_ptr<EffectNode> n(new GainFromBoardEffectNode(4));
    ps.effect_queue.push_back(std::move(n));
  }
  Card::InitBoardSelection(state, player);
  ps.effect_queue.front()->on_action = Card::GainFromBoardHandler;
}

}  // namespace dominion
}  // namespace open_spiel

