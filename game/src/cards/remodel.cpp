#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

// Remodel stage-1 helper: trash one card from hand and switch to board gain.
bool RemodelCard::RemodelTrashFromHand(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::TrashUpToCardsFromHand) return false;
  // Allow early finish without trashing.
  if (action_id == ActionIds::TrashHandSelectFinish()) return true;
  HandSelectionEffectNode* node = nullptr;
  if (!p.effect_queue.empty()) node = dynamic_cast<HandSelectionEffectNode*>(p.effect_queue.front().get());
  if (action_id >= ActionIds::TrashHandBase() && action_id < ActionIds::TrashHandBase() + kNumSupplyPiles) {
    int j = static_cast<int>(action_id - ActionIds::TrashHandBase());
    SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
    SPIEL_CHECK_TRUE(p.hand_counts_[j] > 0);
    SPIEL_CHECK_TRUE(node == nullptr || node->last_selected_original_index() < 0 || j >= node->last_selected_original_index());
    const Card& selected = GetCardSpec(static_cast<CardName>(j));
    int cap = selected.cost_ + 2;
    // Trash selection: remove from hand.
    p.hand_counts_[j] -= 1;
    if (node) node->set_last_selected_original_index(j);
    // Switch to board gain stage: replace current front effect with gain-from-board.
    {
      std::unique_ptr<EffectNode> n(new GainFromBoardEffectNode(cap));
      p.effect_queue.front() = std::move(n);
    }
    Card::InitBoardSelection(st, pl);
    st.player_states_[pl].effect_queue.front()->on_action = Card::GainFromBoardHandler;
    return true;
  }
  return false;
}

void RemodelCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  {
    std::unique_ptr<EffectNode> n(new RemodelTrashEffectNode());
    auto* node = dynamic_cast<HandSelectionEffectNode*>(n.get());
    ps.effect_queue.push_back(std::move(n));
    Card::InitHandSelection(state, player, node, PendingChoice::TrashUpToCardsFromHand);
  }
  ps.effect_queue.front()->on_action = RemodelCard::RemodelTrashFromHand;
}

}  // namespace dominion
}  // namespace open_spiel

