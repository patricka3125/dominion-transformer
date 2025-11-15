#include <map>
#include <algorithm>

#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

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
    // Cellar-specific selection handler: select any number of cards,
    // finish to draw equal to number discarded.
    ps.effect_head->on_action = [](DominionState& st, int pl, Action action_id) {
      auto& p = st.player_states_[pl];
      if (p.pending_choice != PendingChoice::SelectUpToCardsFromHand) return false;
      if (action_id == ActionIds::DiscardFinish()) {
        int draw_n = p.pending_draw_equals_discard ? p.pending_discard_count : 0;
        st.DrawCardsFor(pl, draw_n);
        p.pending_discard_count = 0;
        p.pending_draw_equals_discard = false;
        p.pending_choice = PendingChoice::None;
        return true;
      }
      if (action_id >= ActionIds::DiscardSelectBase() && action_id < ActionIds::DiscardSelectBase() + static_cast<Action>(p.hand_.size())) {
        int idx = static_cast<int>(action_id - ActionIds::DiscardSelectBase());
        if (idx >= 0 && idx < static_cast<int>(p.hand_.size())) {
          CardName cn = p.hand_[idx];
          p.discard_.push_back(cn);
          p.hand_.erase(p.hand_.begin() + idx);
          p.pending_discard_count += 1;
        }
        return true;
      }
      return false;
    };
  }
  // Trigger gain-from-supply effect for Workshop: gain a card costing up to 4.
  if (name_ == std::string("Workshop")) {
    auto& ps = state.player_states_[player];
    ps.effect_head = std::unique_ptr<EffectNode>(new SelectUpToCardsFromBoardNode(4));
    ps.effect_head->onEnter(state, player);
    // Workshop-specific board selection handler: gain from supply up to max cost.
    ps.effect_head->on_action = [](DominionState& st, int pl, Action action_id) {
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
              p.pending_choice = PendingChoice::None;
              p.pending_gain_max_cost = 0;
              return true;
            }
          }
        }
      }
      return false;
    };
  }
}


std::vector<Action> PendingEffectLegalActions(const DominionState& state, int player) {
  std::vector<Action> actions;
  const auto& ps = state.player_states_[player];
  if (ps.pending_choice == PendingChoice::SelectUpToCardsFromHand) {
    for (int i = 0; i < static_cast<int>(ps.hand_.size()) && i < 100; ++i) {
      actions.push_back(ActionIds::DiscardSelect(i));
    }
    actions.push_back(ActionIds::DiscardFinish());
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
  ps.pending_choice = PendingChoice::SelectUpToCardsFromHand;
  ps.pending_discard_count = 0;
  ps.pending_draw_equals_discard = draw_equals_discard_;
}
void SelectUpToCardsFromBoardNode::onEnter(DominionState& state, int player) {
  auto& ps = state.player_states_[player];
  ps.pending_choice = PendingChoice::SelectUpToCardsFromBoard;
  ps.pending_gain_max_cost = max_cost_;
}
} }