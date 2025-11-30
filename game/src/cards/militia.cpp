#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

// Militia handler: opponent discards down to 3 cards, then turn returns to attacker.
bool MilitiaCard::MilitiaOpponentDiscardHandler(DominionState& st, int pl, Action action_id) {
  // Opponent discards down to target hand size (3); finish only at threshold and return turn.
  auto on_select = [](DominionState& st2, int pl2, int j) {
    auto& p2 = st2.player_states_[pl2];
    if (p2.hand_counts_[j] > 0) {
      p2.discard_counts_[j] += 1;
      p2.hand_counts_[j] -= 1;
    }
  };
  auto on_finish = [](DominionState& st2, int pl2) {
    st2.current_player_ = 1 - pl2;
  };
  return Card::GenericHandSelectionHandler(st, pl, action_id,
                                           /*allow_finish=*/false,
                                           /*max_select_count=*/-1,
                                           /*finish_on_target_hand_size=*/true,
                                           ActionIds::DiscardHandBase(),
                                           ActionIds::DiscardHandSelectFinish(),
                                           on_select,
                                           on_finish);
}

void MilitiaCard::applyEffect(DominionState& state, int player) const {
  int opp = 1 - player;
  auto& p_opp = state.player_states_[opp];
  int opp_hand_sz = p_opp.TotalHandSize();
  if (opp_hand_sz > 3) {
    p_opp.effect_queue.clear();
    auto n = EffectNodeFactory::CreateHandSelectionEffect(
        CardName::CARD_Militia,
        PendingChoice::DiscardUpToCardsFromHand);
    p_opp.effect_queue.push_back(std::move(n));
    if (auto* hs = p_opp.FrontEffect()->hand_selection()) {
      hs->set_target_hand_size(3);
    }
    Card::InitHandSelection(state, opp, p_opp.FrontEffect(), PendingChoice::DiscardUpToCardsFromHand);
    p_opp.FrontEffect()->on_action = MilitiaCard::MilitiaOpponentDiscardHandler;
    state.current_player_ = opp;
  }
}

}  // namespace dominion
}  // namespace open_spiel
