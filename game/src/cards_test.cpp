#include <memory>

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

#include "dominion.hpp"

using open_spiel::LoadGame;
using open_spiel::State;
using open_spiel::Game;

namespace open_spiel { namespace dominion {

struct DominionTestHarness {
  static void AddCardToHand(DominionState* s, int player, CardName card) {
    s->player_states_[player].hand_.push_back(card);
  }
  static void SetPhase(DominionState* s, Phase phase) { s->phase_ = phase; }
  static int Actions(DominionState* s) { return s->actions_; }
  static int Buys(DominionState* s) { return s->buys_1; }
  static int Coins(DominionState* s) { return s->coins_; }
  static int HandSize(DominionState* s, int player) { return static_cast<int>(s->player_states_[player].hand_.size()); }
  static int PlayAreaSize(DominionState* s) { return static_cast<int>(s->play_area_.size()); }
  static int DiscardSize(DominionState* s, int player) { return static_cast<int>(s->player_states_[player].discard_.size()); }
};
} }

using open_spiel::dominion::DominionState;
using open_spiel::dominion::CardName;
using open_spiel::dominion::Phase;
using open_spiel::dominion::DominionTestHarness;

static void TestMarket() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Market);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int actions_before = DominionTestHarness::Actions(ds);
  int buys_before = DominionTestHarness::Buys(ds);
  int coins_before = DominionTestHarness::Coins(ds);

  ds->ApplyAction(hand_before - 1);

  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before);
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), actions_before);
  SPIEL_CHECK_EQ(DominionTestHarness::Buys(ds), buys_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Coins(ds), coins_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 1);
}

static void TestVillage() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Village);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int actions_before = DominionTestHarness::Actions(ds);

  ds->ApplyAction(hand_before - 1);

  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before);
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), actions_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 1);
}

static void TestSmithy() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Smithy);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);

  ds->ApplyAction(hand_before - 1);

  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before + 2);
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 1);
}

static void TestFestival() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Festival);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int actions_before = DominionTestHarness::Actions(ds);
  int buys_before = DominionTestHarness::Buys(ds);
  int coins_before = DominionTestHarness::Coins(ds);

  ds->ApplyAction(hand_before - 1);

  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before - 1); // no draws
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), actions_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Buys(ds), buys_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Coins(ds), coins_before + 2);
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 1);
}

static void TestMoat() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Moat);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);

  ds->ApplyAction(hand_before - 1);

  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 1);
}

static void TestCellar() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Cellar);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int actions_before = DominionTestHarness::Actions(ds);
  int discard_before = DominionTestHarness::DiscardSize(ds, 0);

  ds->ApplyAction(hand_before - 1); // play Cellar

  ds->ApplyAction(300 + 0); // discard first card
  ds->ApplyAction(300 + 0); // discard next first card
  ds->ApplyAction(399);     // finish, draw 2

  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before - 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), actions_before);
  SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, 0), discard_before + 2);
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 1);
}

static void TestCellarZeroDiscard() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Cellar);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int actions_before = DominionTestHarness::Actions(ds);
  int discard_before = DominionTestHarness::DiscardSize(ds, 0);

  ds->ApplyAction(hand_before - 1); // play Cellar
  ds->ApplyAction(399);             // finish without discards

  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before - 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), actions_before);
  SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, 0), discard_before);
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 1);
}

static void TestCellarActionStrings() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Cellar);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);

  ds->ApplyAction(hand_before - 1); // play Cellar

  auto actions = ds->LegalActions();
  bool has_finish = false;
  bool has_select0 = false;
  for (auto a : actions) {
    auto s = ds->ActionToString(ds->CurrentPlayer(), a);
    if (s == "DiscardFinish") has_finish = true;
    if (s == "DiscardSelect_0") has_select0 = true;
  }
  SPIEL_CHECK_TRUE(has_finish);
  SPIEL_CHECK_TRUE(has_select0);
}

static void TestCellarOneDiscard() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Cellar);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int actions_before = DominionTestHarness::Actions(ds);
  int discard_before = DominionTestHarness::DiscardSize(ds, 0);

  ds->ApplyAction(hand_before - 1); // play Cellar
  ds->ApplyAction(300 + 0);         // discard one
  ds->ApplyAction(399);             // finish, draw one

  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before - 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), actions_before);
  SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, 0), discard_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 1);
}

static void TestCellarThreeDiscards() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Cellar);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int actions_before = DominionTestHarness::Actions(ds);
  int discard_before = DominionTestHarness::DiscardSize(ds, 0);

  ds->ApplyAction(hand_before - 1); // play Cellar
  ds->ApplyAction(300 + 0);         // discard #1
  ds->ApplyAction(300 + 0);         // discard #2 (index shifts)
  ds->ApplyAction(300 + 0);         // discard #3
  ds->ApplyAction(399);             // finish, draw three

  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before - 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), actions_before);
  SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, 0), discard_before + 3);
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 1);
}

// Verifies that DiscardFinish only appears while a discard effect is active
// and disappears immediately after finishing the effect.
static void TestDiscardFinishVisibility() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Before playing any discard-effect card, DiscardFinish must not be offered.
  {
    auto actions = ds->LegalActions();
    bool has_finish = false;
    for (auto a : actions) {
      if (ds->ActionToString(ds->CurrentPlayer(), a) == "DiscardFinish") {
        has_finish = true;
        break;
      }
    }
    SPIEL_CHECK_FALSE(has_finish);
  }

  // Play Cellar to activate discard effect.
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Cellar);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  ds->ApplyAction(hand_before - 1);

  // DiscardFinish should now be present.
  open_spiel::Action finish_id = -1;
  {
    auto actions = ds->LegalActions();
    for (auto a : actions) {
      if (ds->ActionToString(ds->CurrentPlayer(), a) == "DiscardFinish") {
        finish_id = a;
        break;
      }
    }
    SPIEL_CHECK_TRUE(finish_id != -1);
  }

  // Apply DiscardFinish and check it disappears.
  ds->ApplyAction(finish_id);
  {
    auto actions = ds->LegalActions();
    bool has_finish = false;
    for (auto a : actions) {
      if (ds->ActionToString(ds->CurrentPlayer(), a) == "DiscardFinish") {
        has_finish = true;
        break;
      }
    }
    SPIEL_CHECK_FALSE(has_finish);
  }
}

int main() {
  TestMarket();
  TestVillage();
  TestSmithy();
  TestFestival();
  TestMoat();
  TestCellar();
  TestCellarZeroDiscard();
  TestCellarActionStrings();
  TestCellarOneDiscard();
  TestCellarThreeDiscards();
  TestDiscardFinishVisibility();
  return 0;
}