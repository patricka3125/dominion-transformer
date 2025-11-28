#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

// Cellar handler: select any number of cards in ascending original index; finish draws equal to number discarded.
bool CellarCard::CellarHandSelectHandler(DominionState& st, int pl, Action action_id) {
  // Discard any number; draw equal to discards on finish.
  auto on_select = [](DominionState& st2, int pl2, int j) {
    auto& p2 = st2.player_states_[pl2];
    p2.discard_counts_[j] += 1;
    p2.hand_counts_[j] -= 1;
  };
  auto on_finish = [](DominionState& st2, int pl2) {
    auto& p2 = st2.player_states_[pl2];
    int draw_n = 0;
    if (!p2.effect_queue.empty()) {
      auto* node = p2.effect_queue.front().get();
      auto* hs = node ? node->hand_selection() : nullptr;
      if (hs) draw_n = hs->selection_count_value();
    }
    st2.DrawCardsFor(pl2, draw_n);
  };
  return Card::GenericHandSelectionHandler(st, pl, action_id,
                                           /*allow_finish=*/true,
                                           /*max_select_count=*/-1,
                                           /*finish_on_target_hand_size=*/false,
                                           ActionIds::DiscardHandBase(),
                                           ActionIds::DiscardHandSelectFinish(),
                                           on_select,
                                           on_finish);
}

void CellarCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  {
    std::unique_ptr<EffectNode> n(new CellarEffectNode(PendingChoice::DiscardUpToCardsFromHand));
    ps.effect_queue.push_back(std::move(n));
    Card::InitHandSelection(state, player, ps.effect_queue.front().get(), PendingChoice::DiscardUpToCardsFromHand);
  }
  ps.effect_queue.front()->on_action = CellarCard::CellarHandSelectHandler;
}

}  // namespace dominion
}  // namespace open_spiel
