#include <map>
#include <algorithm>

#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

// Shared helper: handles board gain selection based on pending_gain_max_cost.
bool Card::GainFromBoardHandler(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromBoard) return false;
  if (action_id >= ActionIds::GainSelectBase() && action_id < ActionIds::GainSelectBase() + kNumSupplyPiles) {
    int j = static_cast<int>(action_id - ActionIds::GainSelectBase());
    if (j >= 0 && j < kNumSupplyPiles) {
      if (st.supply_piles_[j] > 0) {
        const Card& spec = GetCardSpec(st.supply_types_[j]);
        if (spec.cost_ <= p.pending_gain_max_cost) {
          st.player_states_[pl].discard_.push_back(st.supply_types_[j]);
          st.supply_piles_[j] -= 1;
          p.ClearBoardSelection();
          p.pending_choice = PendingChoice::None;
          return true;
        }
      }
    }
  }
  return false;
}

// Remodel stage-1 helper: trash one card from hand and switch to board gain.
bool Card::RemodelTrashFromHand(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromHand) return false;
  if (action_id == ActionIds::HandSelectFinish()) return true;
  if (action_id >= ActionIds::HandSelectBase() && action_id < ActionIds::HandSelectBase() + static_cast<Action>(p.hand_.size())) {
    int idx = static_cast<int>(action_id - ActionIds::HandSelectBase());
    if (idx >= 0 && idx < static_cast<int>(p.hand_.size())) {
      int orig_idx = (idx < static_cast<int>(p.pending_hand_original_indices.size())) ? p.pending_hand_original_indices[idx] : idx;
      if (orig_idx > p.pending_last_selected_original_index) {
        const Card& selected = GetCardSpec(p.hand_[idx]);
        int cap = selected.cost_ + 2;
        p.hand_.erase(p.hand_.begin() + idx);
        if (idx < static_cast<int>(p.pending_hand_original_indices.size())) {
          p.pending_hand_original_indices.erase(p.pending_hand_original_indices.begin() + idx);
        }
        p.pending_last_selected_original_index = orig_idx;
        // Switch to board gain stage.
        auto next = std::unique_ptr<EffectNode>(new SelectUpToCardsFromBoardNode(cap));
        st.player_states_[pl].effect_head->next = std::move(next);
        st.player_states_[pl].effect_head = std::move(st.player_states_[pl].effect_head->next);
        st.player_states_[pl].effect_head->onEnter(st, pl);
        st.player_states_[pl].effect_head->on_action = GainFromBoardHandler;
        return true;
      }
    }
    return true;
  }
  return false;
}

// Cellar handler: select any number of cards in ascending original index; finish draws equal to number discarded.
bool Card::CellarHandSelectHandler(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromHand) return false;
  if (action_id == ActionIds::HandSelectFinish()) {
    int draw_n = p.pending_draw_equals_discard ? p.pending_discard_count : 0;
    st.DrawCardsFor(pl, draw_n);
    p.ClearDiscardSelection();
    p.pending_choice = PendingChoice::None;
    return true;
  }
  if (action_id >= ActionIds::HandSelectBase() && action_id < ActionIds::HandSelectBase() + static_cast<Action>(p.hand_.size())) {
    int idx = static_cast<int>(action_id - ActionIds::HandSelectBase());
    if (idx >= 0 && idx < static_cast<int>(p.hand_.size())) {
      int orig_idx = (idx < static_cast<int>(p.pending_hand_original_indices.size())) ? p.pending_hand_original_indices[idx] : idx;
      if (orig_idx > p.pending_last_selected_original_index) {
        CardName cn = p.hand_[idx];
        p.discard_.push_back(cn);
        p.hand_.erase(p.hand_.begin() + idx);
        if (idx < static_cast<int>(p.pending_hand_original_indices.size())) {
          p.pending_hand_original_indices.erase(p.pending_hand_original_indices.begin() + idx);
        }
        p.pending_last_selected_original_index = orig_idx;
        p.pending_discard_count += 1;
      }
    }
    return true;
  }
  return false;
}
// Card spec registry: base set subset sufficient for tests.
static const std::map<CardName, Card>& CardRegistry() {
  static std::map<CardName, Card> reg = {
    {CardName::CARD_Copper,    Card{"Copper",    {CardType::TREASURE}, 0, 1, 0}},
    {CardName::CARD_Silver,    Card{"Silver",    {CardType::TREASURE}, 3, 2, 0}},
    {CardName::CARD_Gold,      Card{"Gold",      {CardType::TREASURE}, 6, 3, 0}},
    {CardName::CARD_Estate,    Card{"Estate",    {CardType::VICTORY},  2, 0, 1}},
    {CardName::CARD_Duchy,     Card{"Duchy",     {CardType::VICTORY},  5, 0, 3}},
    {CardName::CARD_Province,  Card{"Province",  {CardType::VICTORY},  8, 0, 6}},
    {CardName::CARD_Curse,     Card{"Curse",     {CardType::CURSE},    0, 0, -1}},

    {CardName::CARD_Village,   Card{"Village",   {CardType::ACTION},   3, 0, 0, 2, 1, 0}},
    {CardName::CARD_Smithy,    Card{"Smithy",    {CardType::ACTION},   4, 0, 0, 0, 3, 0}},
    {CardName::CARD_Market,    Card{"Market",    {CardType::ACTION},   5, 1, 0, 1, 1, 1}},
    {CardName::CARD_Laboratory,Card{"Laboratory",{CardType::ACTION},   5, 0, 0, 0, 2, 0}},
    {CardName::CARD_Workshop,  Card{"Workshop",  {CardType::ACTION},   3, 0, 0, 0, 0, 0}},
    {CardName::CARD_Cellar,    Card{"Cellar",    {CardType::ACTION},   2, 0, 0, 1, 0, 0}},
    {CardName::CARD_Remodel,   Card{"Remodel",   {CardType::ACTION},   4, 0, 0, 0, 0, 0}},
    {CardName::CARD_Festival,  Card{"Festival",  {CardType::ACTION},   5, 2, 0, 2, 0, 1}},
    {CardName::CARD_Merchant,  Card{"Merchant",  {CardType::ACTION},   3, 0, 0, 1, 1, 0}},
    {CardName::CARD_Mine,      Card{"Mine",      {CardType::ACTION},   5, 0, 0, 0, 0, 0}},
    {CardName::CARD_Moat,      Card{"Moat",      {CardType::ACTION},   2, 0, 0, 0, 2, 0}},
    {CardName::CARD_Artisan,   Card{"Artisan",   {CardType::ACTION},   6, 0, 0, 0, 0, 0}},
    {CardName::CARD_Militia,   Card{"Militia",   {CardType::ACTION},   4, 0, 0, 0, 0, 0}},
    {CardName::CARD_Witch,     Card{"Witch",     {CardType::ACTION},   5, 0, 0, 0, 2, 0}},
    {CardName::CARD_Vassal,    Card{"Vassal",    {CardType::ACTION},   3, 0, 0, 0, 0, 0}},
    {CardName::CARD_Poacher,   Card{"Poacher",   {CardType::ACTION},   4, 1, 0, 1, 1, 0}},
    {CardName::CARD_Bandit,    Card{"Bandit",    {CardType::ACTION},   5, 0, 0, 0, 0, 0}},
    {CardName::CARD_Bureaucrat,Card{"Bureaucrat",{CardType::ACTION},   4, 0, 0, 0, 0, 0}},
    {CardName::CARD_CouncilRoom,Card{"CouncilRoom",{CardType::ACTION}, 5, 0, 0, 0, 4, 1}},
    {CardName::CARD_Gardens,   Card{"Gardens",   {CardType::VICTORY},  4, 0, 0}},
    {CardName::CARD_Harbinger, Card{"Harbinger", {CardType::ACTION},   3, 0, 0, 1, 1, 0}},
    {CardName::CARD_Library,   Card{"Library",   {CardType::ACTION},   5, 0, 0, 0, 0, 0}},
    {CardName::CARD_Moneylender,Card{"Moneylender",{CardType::ACTION}, 4, 0, 0, 0, 0, 0}},
    {CardName::CARD_Sentry,    Card{"Sentry",    {CardType::ACTION},   5, 0, 0, 0, 0, 0}},
    {CardName::CARD_ThroneRoom,Card{"ThroneRoom",{CardType::ACTION},   4, 0, 0, 0, 0, 0}},
  };
  return reg;
}

const Card& GetCardSpec(CardName name) {
  return CardRegistry().at(name);
}

void Card::play(DominionState& state, int player) const {
  state.actions_ += grant_action_;
  state.buys_1 += grant_buy_;
  state.coins_ += value_;
  state.DrawCardsFor(player, grant_draw_);
}

void Card::applyEffect(DominionState& state, int player) const {
  // Trigger generic discard selection effect for cards like Cellar.
  if (name_ == std::string("Cellar")) {
    auto& ps = state.player_states_[player];
    ps.effect_head = std::unique_ptr<EffectNode>(new SelectUpToCardsNode(true));
    ps.effect_head->onEnter(state, player);

    ps.effect_head->on_action = CellarHandSelectHandler;
  }
  // Trigger gain-from-supply effect for Workshop: gain a card costing up to 4.
  if (name_ == std::string("Workshop")) {
    auto& ps = state.player_states_[player];
    ps.effect_head = std::unique_ptr<EffectNode>(new SelectUpToCardsFromBoardNode(4));
    ps.effect_head->onEnter(state, player);
    // Reuse shared handler for gain-from-board.
    ps.effect_head->on_action = GainFromBoardHandler;
  }
  // Trigger Remodel: trash a card from hand, then gain a card costing up to 2 more.
  if (name_ == std::string("Remodel")) {
    auto& ps = state.player_states_[player];
    // First stage: choose exactly one card from hand to trash (remove from game).
    ps.effect_head = std::unique_ptr<EffectNode>(new SelectUpToCardsNode(false));
    ps.effect_head->onEnter(state, player);
    ps.effect_head->on_action = RemodelTrashFromHand;
  }
}


std::vector<Action> PendingEffectLegalActions(const DominionState& state, int player) {
  std::vector<Action> actions;
  const auto& ps = state.player_states_[player];
  if (ps.pending_choice == PendingChoice::SelectUpToCardsFromHand) {
    for (int i = 0; i < static_cast<int>(ps.hand_.size()) && i < 100; ++i) {
      int orig_idx = (i < static_cast<int>(ps.pending_hand_original_indices.size())) ? ps.pending_hand_original_indices[i] : i;
      if (orig_idx > ps.pending_last_selected_original_index) {
        actions.push_back(ActionIds::HandSelect(i));
      }
    }
    actions.push_back(ActionIds::HandSelectFinish());
  }
  if (ps.pending_choice == PendingChoice::SelectUpToCardsFromBoard) {
    for (int j = 0; j < kNumSupplyPiles; ++j) {
      if (state.supply_piles_[j] <= 0) continue;
      const Card& spec = GetCardSpec(state.supply_types_[j]);
      if (spec.cost_ <= ps.pending_gain_max_cost) {
        actions.push_back(ActionIds::GainSelect(j));
      }
    }
  }
  std::sort(actions.begin(), actions.end());
  return actions;
}

}  // namespace dominion
}  // namespace open_spiel
namespace {
}

namespace open_spiel { namespace dominion {
void SelectUpToCardsNode::onEnter(DominionState& state, int player) {
  auto& ps = state.player_states_[player];
  ps.InitHandSelection(draw_equals_discard_);
}
void SelectUpToCardsFromBoardNode::onEnter(DominionState& state, int player) {
  auto& ps = state.player_states_[player];
  ps.InitBoardSelection(max_cost_);
}
} }