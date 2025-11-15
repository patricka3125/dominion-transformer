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
  state.actions_ += grant_action;
  state.buys_1 += grant_buy;
  state.coins_ += value;
  state.DrawCardsFor(player, grant_draw);
}

void Card::applyEffect(DominionState& state, int player) const {
  // Trigger generic discard selection effect for cards like Cellar.
  if (name == std::string("Cellar")) {
    auto& ps = state.player_states_[player];
    ps.pending_choice = PendingChoice::ChooseDiscards;
    ps.pending_discard_draw_count = 0;
  }
}

bool HandlePendingEffectAction(DominionState& state, int player, Action action_id) {
  auto& ps = state.player_states_[player];
  if (ps.pending_choice != PendingChoice::ChooseDiscards) return false;

  if (action_id == ActionIds::DiscardFinish()) {
    state.DrawCardsFor(player, ps.pending_discard_draw_count);
    ps.pending_discard_draw_count = 0;
    ps.pending_choice = PendingChoice::None;
    return true;
  }
  if (action_id >= ActionIds::DiscardSelectBase() && action_id < ActionIds::DiscardSelectBase() + static_cast<Action>(ps.hand_.size())) {
    int idx = static_cast<int>(action_id - ActionIds::DiscardSelectBase());
    if (idx >= 0 && idx < static_cast<int>(ps.hand_.size())) {
      CardName cn = ps.hand_[idx];
      ps.discard_.push_back(cn);
      ps.hand_.erase(ps.hand_.begin() + idx);
      ps.pending_discard_draw_count += 1;
    }
    return true;
  }
  return false;
}

std::vector<Action> PendingEffectLegalActions(const DominionState& state, int player) {
  std::vector<Action> actions;
  const auto& ps = state.player_states_[player];
  if (ps.pending_choice == PendingChoice::ChooseDiscards) {
    for (int i = 0; i < static_cast<int>(ps.hand_.size()) && i < 100; ++i) {
      actions.push_back(ActionIds::DiscardSelect(i));
    }
    actions.push_back(ActionIds::DiscardFinish());
  }
  std::sort(actions.begin(), actions.end());
  return actions;
}

}  // namespace dominion
}  // namespace open_spiel