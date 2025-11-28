#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

// Chapel handler: trash up to four cards from hand; finish early allowed.
bool ChapelCard::ChapelHandTrashHandler(DominionState& st, int pl, Action action_id) {
  // Trash up to 4; finish early allowed.
  auto on_select = [](DominionState& st2, int pl2, int j) {
    auto& p2 = st2.player_states_[pl2];
    if (p2.hand_counts_[j] > 0) p2.hand_counts_[j] -= 1;
  };
  auto on_finish = [](DominionState&, int) {};
  return Card::GenericHandSelectionHandler(st, pl, action_id,
                                           /*allow_finish=*/true,
                                           /*max_select_count=*/4,
                                           /*finish_on_target_hand_size=*/false,
                                           ActionIds::TrashHandBase(),
                                           ActionIds::TrashHandSelectFinish(),
                                           on_select,
                                           on_finish);
}

void ChapelCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  {
    std::unique_ptr<EffectNode> n(new ChapelEffectNode(PendingChoice::TrashUpToCardsFromHand));
    ps.effect_queue.push_back(std::move(n));
    Card::InitHandSelection(state, player, ps.effect_queue.front().get(), PendingChoice::TrashUpToCardsFromHand);
  }
  ps.effect_queue.front()->on_action = ChapelCard::ChapelHandTrashHandler;
}

}  // namespace dominion
}  // namespace open_spiel
