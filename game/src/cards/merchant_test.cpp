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
static int Actions(DominionState* s) { return s->actions_; }
static int Coins(DominionState* s) { return s->coins_; }

static void TestMerchantSilverBonusOnce() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  AddCardToHand(ds, 0, CardName::CARD_Merchant);
  AddCardToHand(ds, 0, CardName::CARD_Merchant);
  SetPhase(ds, Phase::actionPhase);

  int actions_before = Actions(ds);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Merchant)));
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Merchant)));
  SPIEL_CHECK_EQ(Actions(ds), actions_before - 2 + 2); // Merchants grant +1 action each

  ds->ApplyAction(open_spiel::dominion::ActionIds::EndActions());
  SPIEL_CHECK_EQ(Coins(ds), 0);

  AddCardToHand(ds, 0, CardName::CARD_Silver);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Silver)));
  SPIEL_CHECK_EQ(Coins(ds), 2 + 2);

  AddCardToHand(ds, 0, CardName::CARD_Silver);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Silver)));
  SPIEL_CHECK_EQ(Coins(ds), 2 + 2 + 2);
}

void RunMerchantTests() {
  TestMerchantSilverBonusOnce();
}

} }

