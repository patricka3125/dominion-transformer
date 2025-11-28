#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

// ThroneRoomEffectNode methods moved to src/effects.cpp

// Throne Room selection: choose one action card from hand; first play executes
// grants and effect without spending an action; second play re-applies effect only.
bool ThroneRoomCard::ThroneRoomSelectActionHandler(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::PlayActionFromHand) return false;
  ThroneRoomEffectNode* node = nullptr;
  if (!p.effect_queue.empty()) node = dynamic_cast<ThroneRoomEffectNode*>(p.effect_queue.front().get());
  if (action_id == ActionIds::ThroneHandSelectFinish()) {
    // No selection: do nothing, end effect.
    if (node) node->FinishSelection(st, pl);
    return true;
  }
  if (action_id >= 0 && action_id < ActionIds::MaxHandSize()) {
    int j = static_cast<int>(action_id);
    SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
    SPIEL_CHECK_TRUE(p.hand_counts_[j] > 0);
    const auto* hs = node ? node->hand_selection() : nullptr;
    SPIEL_CHECK_TRUE(hs == nullptr || hs->last_selected_original_index_value() < 0 || j >= hs->last_selected_original_index_value());
    CardName cn = static_cast<CardName>(j);
    const Card& spec = GetCardSpec(cn);
    // Must be an action card; ignore non-action selections.
    if (std::find(spec.types_.begin(), spec.types_.end(), CardType::ACTION) == spec.types_.end()) {
      return true;
    }
    // First play: move to play area, do standard grants, no action decrement here.
    p.hand_counts_[j] -= 1;
    st.play_area_.push_back(cn);
    // If the selected card is another Throne Room, chain a new selection node for choosing an action.
    if (cn == CardName::CARD_ThroneRoom) {
      if (node) node->StartChain(st, pl);
    } else {
      // Otherwise, apply the card's own effect twice (Throne effect) and decrement depth once.
      spec.Play(st, pl);
      spec.Play(st, pl);
      if (node) node->ContinueOrFinish(st, pl);
    }
    return true;
  }
  return false;
}

void ThroneRoomCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  {
    std::unique_ptr<EffectNode> n(new ThroneRoomEffectNode());
    ps.effect_queue.push_back(std::move(n));
  }
  auto* node = dynamic_cast<ThroneRoomEffectNode*>(ps.effect_queue.front().get());
  if (node) node->StartChain(state, player);
}

}  // namespace dominion
}  // namespace open_spiel
