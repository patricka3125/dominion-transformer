#include <memory>

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel { namespace dominion {

static void AddCardToHand(DominionState* s, int player, CardName card) {
  int idx = static_cast<int>(card);
  if (idx >= 0 && idx < kNumSupplyPiles) s->player_states_[player].hand_counts_[idx] += 1;
}
static void SetPhase(DominionState* s, Phase phase) { s->phase_ = phase; }
static int Coins(DominionState* s) { return s->coins_; }

static void TestMoneylenderTrashesOneCopperAndGrantsCoins() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Controlled hand: clear then add one Copper and Moneylender.
  ds->player_states_[0].hand_counts_.fill(0);
  AddCardToHand(ds, 0, CardName::CARD_Copper);
  AddCardToHand(ds, 0, CardName::CARD_Moneylender);
  SetPhase(ds, Phase::actionPhase);

  int coins_before = Coins(ds);
  int copper_idx = static_cast<int>(CardName::CARD_Copper);
  int copper_before = ds->player_states_[0].hand_counts_[copper_idx];
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Moneylender)));
  SPIEL_CHECK_EQ(ds->player_states_[0].hand_counts_[copper_idx], copper_before - 1);
  SPIEL_CHECK_EQ(Coins(ds), coins_before + 3);
}

static void TestMoneylenderNoCopperNoOp() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Controlled hand: clear then add only Moneylender, no Copper.
  ds->player_states_[0].hand_counts_.fill(0);
  AddCardToHand(ds, 0, CardName::CARD_Moneylender);
  SetPhase(ds, Phase::actionPhase);
  int coins_before = Coins(ds);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Moneylender)));
  SPIEL_CHECK_EQ(Coins(ds), coins_before);
}

static void TestMoneylenderMultipleCoppersOnlyOneTrashed() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Controlled hand: clear then add two Coppers and Moneylender.
  ds->player_states_[0].hand_counts_.fill(0);
  AddCardToHand(ds, 0, CardName::CARD_Copper);
  AddCardToHand(ds, 0, CardName::CARD_Copper);
  AddCardToHand(ds, 0, CardName::CARD_Moneylender);
  SetPhase(ds, Phase::actionPhase);
  int coins_before = Coins(ds);
  int copper_idx = static_cast<int>(CardName::CARD_Copper);
  int copper_before = ds->player_states_[0].hand_counts_[copper_idx];
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Moneylender)));
  SPIEL_CHECK_EQ(ds->player_states_[0].hand_counts_[copper_idx], copper_before - 1);
  SPIEL_CHECK_EQ(Coins(ds), coins_before + 3);
}

void RunMoneylenderTests() {
  TestMoneylenderTrashesOneCopperAndGrantsCoins();
  TestMoneylenderNoCopperNoOp();
  TestMoneylenderMultipleCoppersOnlyOneTrashed();
}

} }
