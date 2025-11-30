#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

void WorkshopCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  auto n = EffectNodeFactory::CreateGainEffect(CardName::CARD_Workshop, 4);
  ps.effect_queue.push_back(std::move(n));
  Card::InitBoardSelection(state, player);
  ps.FrontEffect()->on_action = Card::GainFromBoardHandler;
}

}  // namespace dominion
}  // namespace open_spiel
