#include "effects.hpp"
#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

CellarEffectNode::CellarEffectNode(PendingChoice choice,
                                   const HandSelectionStruct* hs) {
  enforce_ascending = true;
  if (hs) hand_ = *hs;
  if (choice == PendingChoice::DiscardUpToCardsFromHand) {
    on_action = CellarCard::CellarHandSelectHandler;
  }
}

ChapelEffectNode::ChapelEffectNode(PendingChoice choice,
                                   const HandSelectionStruct* hs) {
  enforce_ascending = true;
  if (hs) hand_ = *hs;
  if (choice == PendingChoice::TrashUpToCardsFromHand) {
    on_action = ChapelCard::ChapelHandTrashHandler;
  }
}

RemodelTrashEffectNode::RemodelTrashEffectNode(PendingChoice choice,
                                               const HandSelectionStruct* hs) {
  enforce_ascending = true;
  if (hs) hand_ = *hs;
  if (choice == PendingChoice::TrashUpToCardsFromHand) {
    on_action = RemodelCard::RemodelTrashFromHand;
  }
}

MilitiaEffectNode::MilitiaEffectNode(PendingChoice choice,
                                     const HandSelectionStruct* hs) {
  enforce_ascending = false;
  if (hs) hand_ = *hs;
  if (choice == PendingChoice::DiscardUpToCardsFromHand) {
    on_action = MilitiaCard::MilitiaOpponentDiscardHandler;
  }
}

ThroneRoomEffectNode::ThroneRoomEffectNode(int depth) {
  enforce_ascending = false;
  for (int i = 0; i < depth; ++i) increment_throne_depth();
}

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
    s.hand = *hs;
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
      out = std::unique_ptr<EffectNode>(new CellarEffectNode(pending_choice, &s.hand));
      break;
    }
    case CardName::CARD_Chapel: {
      out = std::unique_ptr<EffectNode>(new ChapelEffectNode(pending_choice, &s.hand));
      break;
    }
    case CardName::CARD_Remodel: {
      if (pending_choice == PendingChoice::TrashUpToCardsFromHand) {
        out = std::unique_ptr<EffectNode>(new RemodelTrashEffectNode(pending_choice, &s.hand));
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
      out = std::unique_ptr<EffectNode>(new MilitiaEffectNode(pending_choice, &s.hand));
      break;
    }
    case CardName::CARD_ThroneRoom: {
      auto n = std::unique_ptr<ThroneRoomEffectNode>(new ThroneRoomEffectNode(s.throne_select_depth));
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

// EffectNodeFactory implementation
std::unique_ptr<EffectNode> EffectNodeFactory::CreateHandSelectionEffect(
    CardName card,
    PendingChoice choice,
    const HandSelectionStruct* hs) {
  std::unique_ptr<EffectNode> node;
  switch (card) {
    case CardName::CARD_Cellar:
      node = std::unique_ptr<EffectNode>(new CellarEffectNode(choice, hs));
      break;
    case CardName::CARD_Chapel:
      node = std::unique_ptr<EffectNode>(new ChapelEffectNode(choice, hs));
      break;
    case CardName::CARD_Remodel:
      node = std::unique_ptr<EffectNode>(new RemodelTrashEffectNode(choice, hs));
      break;
    case CardName::CARD_Militia:
      node = std::unique_ptr<EffectNode>(new MilitiaEffectNode(choice, hs));
      break;
    default:
      // Unsupported card type for hand selection effects
      break;
  }
  return node;
}

std::unique_ptr<EffectNode> EffectNodeFactory::CreateGainEffect(
    CardName card,
    int max_cost) {
  std::unique_ptr<EffectNode> node;
  switch (card) {
    case CardName::CARD_Workshop:
      node = std::unique_ptr<EffectNode>(new WorkshopEffectNode(max_cost));
      break;
    case CardName::CARD_Remodel:
      node = std::unique_ptr<EffectNode>(new RemodelGainEffectNode(max_cost));
      break;
    default:
      // Unsupported card type for gain effects
      break;
  }
  return node;
}

std::unique_ptr<EffectNode> EffectNodeFactory::CreateThroneRoomEffect(int depth) {
  return std::unique_ptr<EffectNode>(new ThroneRoomEffectNode(depth));
}

std::unique_ptr<EffectNode> EffectNodeFactory::Create(
    CardName card,
    PendingChoice choice,
    const HandSelectionStruct* hs,
    int extra_param) {
  // For throne room, extra_param represents depth
  if (card == CardName::CARD_ThroneRoom) {
    return CreateThroneRoomEffect(extra_param);
  }

  // For gain effects
  if (choice == PendingChoice::SelectUpToCardsFromBoard) {
    return CreateGainEffect(card, extra_param);
  }

  // For hand selection effects
  return CreateHandSelectionEffect(card, choice, hs);
}

} // namespace dominion
} // namespace open_spiel
