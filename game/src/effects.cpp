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
  if (!p.effect_queue.empty()) p.effect_queue.pop_front();
}

} // namespace dominion
} // namespace open_spiel

namespace open_spiel {
namespace dominion {

EffectNodeStructContents EffectNodeToStruct(const EffectNode& node) {
  EffectNodeStructContents s;
  if (dynamic_cast<const CellarEffectNode*>(&node)) {
    s.kind = static_cast<int>(CardName::CARD_Cellar);
  } else if (dynamic_cast<const ChapelEffectNode*>(&node)) {
    s.kind = static_cast<int>(CardName::CARD_Chapel);
  } else if (dynamic_cast<const RemodelTrashEffectNode*>(&node)) {
    s.kind = static_cast<int>(CardName::CARD_Remodel);
  } else if (dynamic_cast<const RemodelGainEffectNode*>(&node)) {
    s.kind = static_cast<int>(CardName::CARD_Remodel);
  } else if (dynamic_cast<const MilitiaEffectNode*>(&node)) {
    s.kind = static_cast<int>(CardName::CARD_Militia);
  } else if (auto tr = dynamic_cast<const ThroneRoomEffectNode*>(&node)) {
    s.kind = static_cast<int>(CardName::CARD_ThroneRoom);
    s.throne_select_depth = tr->throne_depth();
  } else if (dynamic_cast<const WorkshopEffectNode*>(&node)) {
    s.kind = static_cast<int>(CardName::CARD_Workshop);
  }
  if (auto hs = node.hand_selection()) {
    s.hand_target_hand_size = hs->target_hand_size_value();
    s.hand_last_selected_original_index = hs->last_selected_original_index_value();
    s.hand_selection_count = hs->selection_count_value();
  }
  if (auto gs = node.gain_from_board()) {
    s.gain_max_cost = gs->max_cost;
  }
  return s;
}

std::unique_ptr<EffectNode> EffectNodeFromStruct(const EffectNodeStructContents& s,
                                                 PendingChoice pending_choice) {
  std::unique_ptr<EffectNode> out;
  auto card_kind = static_cast<CardName>(s.kind);
  switch (card_kind) {
    case CardName::CARD_Cellar: {
      auto n = std::unique_ptr<CellarEffectNode>(new CellarEffectNode());
      if (auto hs = n->hand_selection()) {
        hs->set_target_hand_size(s.hand_target_hand_size);
        hs->set_last_selected_original_index(s.hand_last_selected_original_index);
        hs->set_selection_count(s.hand_selection_count);
      }
      if (pending_choice == PendingChoice::DiscardUpToCardsFromHand) {
        n->on_action = CellarCard::CellarHandSelectHandler;
      }
      out = std::unique_ptr<EffectNode>(std::move(n));
      break;
    }
    case CardName::CARD_Chapel: {
      auto n = std::unique_ptr<ChapelEffectNode>(new ChapelEffectNode());
      if (auto hs = n->hand_selection()) {
        hs->set_target_hand_size(s.hand_target_hand_size);
        hs->set_last_selected_original_index(s.hand_last_selected_original_index);
        hs->set_selection_count(s.hand_selection_count);
      }
      if (pending_choice == PendingChoice::TrashUpToCardsFromHand) {
        n->on_action = ChapelCard::ChapelHandTrashHandler;
      }
      out = std::unique_ptr<EffectNode>(std::move(n));
      break;
    }
    case CardName::CARD_Remodel: {
      if (pending_choice == PendingChoice::TrashUpToCardsFromHand) {
        auto n = std::unique_ptr<RemodelTrashEffectNode>(new RemodelTrashEffectNode());
        if (auto hs = n->hand_selection()) {
          hs->set_target_hand_size(s.hand_target_hand_size);
          hs->set_last_selected_original_index(s.hand_last_selected_original_index);
          hs->set_selection_count(s.hand_selection_count);
        }
        n->on_action = RemodelCard::RemodelTrashFromHand;
        out = std::unique_ptr<EffectNode>(std::move(n));
      } else {
        auto n = std::unique_ptr<RemodelGainEffectNode>(new RemodelGainEffectNode(s.gain_max_cost));
        if (pending_choice == PendingChoice::SelectUpToCardsFromBoard) {
          n->on_action = Card::GainFromBoardHandler;
        }
        out = std::unique_ptr<EffectNode>(std::move(n));
      }
      
      break;
    }
    case CardName::CARD_Militia: {
      auto n = std::unique_ptr<MilitiaEffectNode>(new MilitiaEffectNode());
      if (auto hs = n->hand_selection()) {
        hs->set_target_hand_size(s.hand_target_hand_size);
        hs->set_last_selected_original_index(s.hand_last_selected_original_index);
        hs->set_selection_count(s.hand_selection_count);
      }
      if (pending_choice == PendingChoice::DiscardUpToCardsFromHand) {
        n->on_action = MilitiaCard::MilitiaOpponentDiscardHandler;
      }
      out = std::unique_ptr<EffectNode>(std::move(n));
      break;
    }
    case CardName::CARD_ThroneRoom: {
      auto n = std::unique_ptr<ThroneRoomEffectNode>(new ThroneRoomEffectNode());
      for (int i = 0; i < s.throne_select_depth; ++i) n->increment_throne_depth();
      if (auto hs = n->hand_selection()) {
        hs->set_target_hand_size(s.hand_target_hand_size);
        hs->set_last_selected_original_index(s.hand_last_selected_original_index);
        hs->set_selection_count(s.hand_selection_count);
      }
      if (pending_choice == PendingChoice::PlayActionFromHand) {
        n->on_action = ThroneRoomCard::ThroneRoomSelectActionHandler;
      }
      out = std::unique_ptr<EffectNode>(std::move(n));
      break;
    }
    case CardName::CARD_Workshop: {
      auto n = std::unique_ptr<WorkshopEffectNode>(new WorkshopEffectNode(s.gain_max_cost));
      if (pending_choice == PendingChoice::SelectUpToCardsFromBoard) {
        n->on_action = Card::GainFromBoardHandler;
      }
      out = std::unique_ptr<EffectNode>(std::move(n));
      break;
    }
  }
  return out;
}

} // namespace dominion
} // namespace open_spiel
