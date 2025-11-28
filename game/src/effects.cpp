#include "effects.hpp"
#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

// Begin a fresh selection for an action card from hand.
// - Sets pending choice to PlayActionFromHand
// - Installs ThroneRoomSelectActionHandler on the front node
void ThroneRoomEffectNode::BeginSelection(DominionState& state, int player) {
  Card::InitHandSelection(state, player, this, PendingChoice::PlayActionFromHand);
  state.player_states_[player].effect_queue.front()->on_action = ThroneRoomCard::ThroneRoomSelectActionHandler;
}

// Increment chain depth and begin another selection.
// This models picking Throne Room and chaining until a non-Throne action is chosen.
void ThroneRoomEffectNode::StartChain(DominionState& state, int player) {
  increment_throne_depth();
  BeginSelection(state, player);
}

// Decrement depth; finish if zero, else restart selection.
// Called after double-playing a non-Throne action in the chain.
void ThroneRoomEffectNode::ContinueOrFinish(DominionState& state, int player) {
  decrement_throne_depth();
  if (throne_depth() == 0) {
    FinishSelection(state, player);
  } else {
    BeginSelection(state, player);
  }
}

// Clear pending choice and finish the effect.
// Resets discard selection UI and marks the effect complete.
void ThroneRoomEffectNode::FinishSelection(DominionState& state, int player) {
  auto& p = state.player_states_[player];
  p.ClearDiscardSelection();
  p.pending_choice = PendingChoice::None;
}

} // namespace dominion
} // namespace open_spiel

