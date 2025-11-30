#include <memory>

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

#include "dominion.hpp"
#include "actions.hpp"

using open_spiel::LoadGame;
using open_spiel::State;
using open_spiel::Game;

namespace open_spiel { namespace dominion {

struct DominionTestHarness {
  static void AddCardToHand(DominionState* s, int player, CardName card) {
    int idx = static_cast<int>(card);
    if (idx >= 0 && idx < kNumSupplyPiles) s->player_states_[player].hand_counts_[idx] += 1;
  }
  static void AddCardToDeck(DominionState* s, int player, CardName card) {
    s->player_states_[player].deck_.push_back(card);
  }
  static int FindHandIndexOf(DominionState* s, int player, CardName card) {
    int idx = static_cast<int>(card);
    auto& hc = s->player_states_[player].hand_counts_;
    return (idx >= 0 && idx < kNumSupplyPiles && hc[idx] > 0) ? idx : -1;
  }
  static void SetPhase(DominionState* s, Phase phase) { s->phase_ = phase; }
  static int Actions(DominionState* s) { return s->actions_; }
  static int Buys(DominionState* s) { return s->buys_; }
  static int Coins(DominionState* s) { return s->coins_; }
  static int HandSize(DominionState* s, int player) { int c=0; for(int j=0;j<kNumSupplyPiles;++j) c+=s->player_states_[player].hand_counts_[j]; return c; }
  static int PlayAreaSize(DominionState* s) { return static_cast<int>(s->play_area_.size()); }
  static int DeckSize(DominionState* s, int player) { return static_cast<int>(s->player_states_[player].deck_.size()); }
  static int DiscardSize(DominionState* s, int player) { int c=0; for(int j=0;j<kNumSupplyPiles;++j) c+=s->player_states_[player].discard_counts_[j]; return c; }
  static int SupplyCount(DominionState* s, int j) { return s->supply_piles_[j]; }
};
} }

using open_spiel::dominion::DominionState;
using open_spiel::dominion::CardName;
using open_spiel::dominion::Phase;
using open_spiel::dominion::DominionTestHarness;

namespace open_spiel { namespace dominion { void RunChapelTests(); } }
namespace open_spiel { namespace dominion { void RunCellarTests(); } }
namespace open_spiel { namespace dominion { void RunWorkshopTests(); } }
namespace open_spiel { namespace dominion { void RunRemodelTests(); } }
namespace open_spiel { namespace dominion { void RunMilitiaTests(); } }
namespace open_spiel { namespace dominion { void RunWitchTests(); } }
namespace open_spiel { namespace dominion { void RunThroneRoomTests(); } }
namespace open_spiel { namespace dominion { void RunChapelJsonRoundTrip(); } }
namespace open_spiel { namespace dominion { void RunCellarJsonRoundTrip(); } }
namespace open_spiel { namespace dominion { void RunWorkshopJsonRoundTrip(); } }
namespace open_spiel { namespace dominion { void RunRemodelJsonRoundTrip(); } }
namespace open_spiel { namespace dominion { void RunMilitiaJsonRoundTrip(); } }
namespace open_spiel { namespace dominion { void RunThroneRoomJsonRoundTrip(); } }
namespace open_spiel { namespace dominion { void RunWitchJsonRoundTrip(); } }
namespace open_spiel { namespace dominion { void RunCellarTests(); } }
namespace open_spiel { namespace dominion { void RunWorkshopTests(); } }
namespace open_spiel { namespace dominion { void RunRemodelTests(); } }
namespace open_spiel { namespace dominion { void RunMilitiaTests(); } }
namespace open_spiel { namespace dominion { void RunWitchTests(); } }
namespace open_spiel { namespace dominion { void RunThroneRoomTests(); } }

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

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Market)));

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

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Village)));

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

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Smithy)));

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

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Festival)));

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

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Moat)));

  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 1);
}



// Verifies that DiscardFinish only appears while a discard effect is active
// and disappears immediately after finishing the effect.
static void TestDiscardFinishVisibility() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Before playing any discard-effect card, DiscardHandSelectFinish must not be offered.
  {
    auto actions = ds->LegalActions();
    bool has_finish = std::find(actions.begin(), actions.end(), open_spiel::dominion::ActionIds::DiscardHandSelectFinish()) != actions.end();
    SPIEL_CHECK_FALSE(has_finish);
  }

  // Play Cellar to activate discard effect.
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Cellar);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Cellar)));

  // DiscardHandSelectFinish should now be present.
  open_spiel::Action finish_id = -1;
  {
    auto actions = ds->LegalActions();
    auto it = std::find(actions.begin(), actions.end(), open_spiel::dominion::ActionIds::DiscardHandSelectFinish());
    if (it != actions.end()) finish_id = *it;
    SPIEL_CHECK_TRUE(finish_id != -1);
  }

  // Apply DiscardFinish and check it disappears.
  ds->ApplyAction(finish_id);
  {
    auto actions = ds->LegalActions();
    bool has_finish = std::find(actions.begin(), actions.end(), open_spiel::dominion::ActionIds::DiscardHandSelectFinish()) != actions.end();
    SPIEL_CHECK_FALSE(has_finish);
  }
}

static void TestWorkshopGain() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Workshop);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int discard_before = DominionTestHarness::DiscardSize(ds, 0);
  int smithy_idx = static_cast<int>(CardName::CARD_Smithy);
  int smithy_pile_before = DominionTestHarness::SupplyCount(ds, smithy_idx);

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Workshop))); // play Workshop

  // Find GainSelect for Smithy (enumerator index, cost 4) and apply it.
  open_spiel::Action gain_smithy = -1;
  for (auto a : ds->LegalActions()) {
    if (a == open_spiel::dominion::ActionIds::GainSelect(smithy_idx)) {
      gain_smithy = a;
      break;
    }
  }
  SPIEL_CHECK_TRUE(gain_smithy != -1);
  ds->ApplyAction(gain_smithy);

  SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, 0), discard_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::SupplyCount(ds, smithy_idx), smithy_pile_before - 1);
}

static void TestActionToStringAppendsCardNames() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  int estate_idx = static_cast<int>(CardName::CARD_Estate);
  int copper_idx = static_cast<int>(CardName::CARD_Copper);
  int smithy_idx = static_cast<int>(CardName::CARD_Smithy);

  std::string s1 = ds->ActionToString(ds->CurrentPlayer(), open_spiel::dominion::ActionIds::DiscardHandSelect(estate_idx));
  SPIEL_CHECK_TRUE(s1.find("(Estate)") != std::string::npos);

  std::string s2 = ds->ActionToString(ds->CurrentPlayer(), open_spiel::dominion::ActionIds::TrashHandSelect(copper_idx));
  SPIEL_CHECK_TRUE(s2.find("(Copper)") != std::string::npos);

  std::string s3 = ds->ActionToString(ds->CurrentPlayer(), open_spiel::dominion::ActionIds::GainSelect(smithy_idx));
  SPIEL_CHECK_TRUE(s3.find("(Smithy)") != std::string::npos);
}


static void TestRemodelTrashThenGain() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Ensure a known trash target (Estate, cost 2) is in hand.
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Estate);
  // Add Remodel last so it is played.
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Remodel);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int discard_before = DominionTestHarness::DiscardSize(ds, 0);
  int smithy_idx2 = static_cast<int>(CardName::CARD_Smithy);
  int smithy_pile_before = DominionTestHarness::SupplyCount(ds, smithy_idx2);

  // Play Remodel (last card in hand).
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Remodel)));

  // Trash the previously appended Estate (now last card in hand).
  ds->ApplyAction(open_spiel::dominion::ActionIds::TrashHandSelect(static_cast<int>(CardName::CARD_Estate)));

  // Gain a Smithy (cost 4 <= 2 + 2) via board selection.
  open_spiel::Action gain_smithy = -1;
  for (auto a : ds->LegalActions()) {
    if (a == open_spiel::dominion::ActionIds::GainSelect(smithy_idx2)) {
      gain_smithy = a;
      break;
    }
  }
  SPIEL_CHECK_TRUE(gain_smithy != -1);
  ds->ApplyAction(gain_smithy);

  SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, 0), discard_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::SupplyCount(ds, smithy_idx2), smithy_pile_before - 1);
}

static void TestMilitiaOpponentDiscardsToThree() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Current player plays Militia.
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Militia);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before_p0 = DominionTestHarness::HandSize(ds, 0);
  int hand_before_p1 = DominionTestHarness::HandSize(ds, 1); // typically 5

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Militia)));

  // Opponent must now discard 2 to reach 3. Current player switches to 1.
  SPIEL_CHECK_EQ(ds->CurrentPlayer(), 1);
  {
    auto la = ds->LegalActions();
    bool has_finish = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::DiscardHandSelectFinish()) != la.end();
    SPIEL_CHECK_FALSE(has_finish);
  }
  ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelect(0));
  {
    auto la = ds->LegalActions();
    bool has_finish = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::DiscardHandSelectFinish()) != la.end();
    SPIEL_CHECK_FALSE(has_finish);
  }
  ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelect(0));

  // After discarding, turn should return to player 0.
  SPIEL_CHECK_EQ(ds->CurrentPlayer(), 0);
  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 1), 3);
}

static void TestWitchGivesCurseToOpponent() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Add Witch to current player's hand and play it.
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Witch);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before_p0 = DominionTestHarness::HandSize(ds, 0);
  int curse_idx = static_cast<int>(CardName::CARD_Curse);
  int curse_supply_before = DominionTestHarness::SupplyCount(ds, curse_idx);
  int opp_discard_before = DominionTestHarness::DiscardSize(ds, 1);

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Witch)));

  // Witch grants +2 cards; net hand change is -1 + 2 = +1.
  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before_p0 + 1);
  // Opponent gains a Curse if available.
  SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, 1), opp_discard_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::SupplyCount(ds, curse_idx), curse_supply_before - 1);
}

static void TestThroneRoomPlaysCardTwiceEffectOnlySecond() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Put Smithy and Throne Room into hand; play Throne Room and select Smithy.
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Smithy);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
  // Ensure sufficient deck size to draw 6 cards across both plays.
  for (int i = 0; i < 10; ++i) DominionTestHarness::AddCardToDeck(ds, 0, CardName::CARD_Copper);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int actions_before = DominionTestHarness::Actions(ds);
  int deck_before = DominionTestHarness::DeckSize(ds, 0);

  // Play Throne Room (last in hand).
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));

  // Select Smithy (now last index in hand after removing Throne Room).
  int hand_after_play = DominionTestHarness::HandSize(ds, 0);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Smithy)));

  int deck_after = DominionTestHarness::DeckSize(ds, 0);
  int draws = deck_before - deck_after;
  SPIEL_CHECK_EQ(draws, 6);
  // First play of Smithy removes it (-1); Throne removes itself (-1).
  // Final hand size should reflect removals plus draws.
  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before - DominionTestHarness::PlayAreaSize(ds) + draws);
  // Actions should decrease by 1 for Throne Room only; Smithy play should not consume an action.
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), actions_before - 1);
}

static void TestThroneRoomIntoThroneRoomTwoActionsTwice() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Hand: Smithy A, Smithy B, Throne B, Throne A (in that order; Throne A last).
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Smithy);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Smithy);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
  // Ensure large deck for draws: 30 coppers
  for (int i = 0; i < 30; ++i) DominionTestHarness::AddCardToDeck(ds, 0, CardName::CARD_Copper);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);

  int hand_before = DominionTestHarness::HandSize(ds, 0);
  int deck_before = DominionTestHarness::DeckSize(ds, 0);

  // Play Throne A (last index)
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));
  // Select Throne B: choose any PlayHandIndex in legal actions (only actions are offered).
  {
    int idx = DominionTestHarness::FindHandIndexOf(ds, 0, CardName::CARD_ThroneRoom);
    SPIEL_CHECK_TRUE(idx != -1);
    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(idx));
  }
  // Throne B selection: choose Smithy A (now last index)
  {
    int idx = DominionTestHarness::FindHandIndexOf(ds, 0, CardName::CARD_Smithy);
    SPIEL_CHECK_TRUE(idx != -1);
    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(idx));
  }
  // Throne B second play: choose Smithy B (now last index)
  {
    int idx = DominionTestHarness::FindHandIndexOf(ds, 0, CardName::CARD_Smithy);
    SPIEL_CHECK_TRUE(idx != -1);
    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(idx));
  }
  int deck_after_all = DominionTestHarness::DeckSize(ds, 0);
  int total_draws = deck_before - deck_after_all;
  SPIEL_CHECK_EQ(total_draws, 12);
  // Remove count equals play area size; final hand size reflects removals plus draws.
  SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, 0), hand_before - DominionTestHarness::PlayAreaSize(ds) + total_draws);
  // Both Smithies and both Thrones should be in play area.
  SPIEL_CHECK_EQ(DominionTestHarness::PlayAreaSize(ds), 4);
  // No further selection is active.
  {
    auto la = ds->LegalActions();
    bool has_throne_finish = std::find(
        la.begin(), la.end(), open_spiel::dominion::ActionIds::ThroneHandSelectFinish())
        != la.end();
    SPIEL_CHECK_FALSE(has_throne_finish);
  }
}

static void TestThroneRoomAllowsNoSelection() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Only Throne Room in hand.
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hb = DominionTestHarness::HandSize(ds, 0);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));
  // During selection, ThroneHandSelectFinish should be legal.
  auto la = ds->LegalActions();
  bool has_finish = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::ThroneHandSelectFinish()) != la.end();
  SPIEL_CHECK_TRUE(has_finish);
}

static void TestThroneRoomNoActionsShowsNoHandSelect() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Put only non-action cards in hand plus Throne Room last.
  // Clear initial hand to isolate.
  // Add 4 Coppers then ThroneRoom.
  for (int i = 0; i < 4; ++i) DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Copper);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);
  int hb = DominionTestHarness::HandSize(ds, 0);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));
  auto la = ds->LegalActions();
  // No PlayHandIndex(i) should be present (only finish).
  bool any_select = false;
  for (auto a : la) {
    std::string s = ds->ActionToString(ds->CurrentPlayer(), a);
    if (s.find("PlayHandIndex_") == 0) any_select = true;
  }
  SPIEL_CHECK_FALSE(any_select);
}

// Verify that Throne Room chain selection does not enforce ascending
// last_selected_original_index ordering: after selecting a Throne Room,
// an ACTION card with a lower enumerator index (e.g., Smithy) remains selectable.
static void TestThroneRoomIgnoresAscendingConstraint() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Hand: ThroneRoom (A), ThroneRoom (B), Smithy. Smithy has a lower index
  // than ThroneRoom in CardName ordering.
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Smithy);
  DominionTestHarness::SetPhase(ds, Phase::actionPhase);

  // Play ThroneRoom (A) to start the chain.
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));

  // First selection: pick ThroneRoom (B).
  {
    int idx = DominionTestHarness::FindHandIndexOf(ds, 0, CardName::CARD_ThroneRoom);
    SPIEL_CHECK_TRUE(idx != -1);
    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(idx));
  }

  // Now depth > 0: Smithy should still be selectable even if its index is lower.
  auto actions = ds->LegalActions();
  bool can_select_smithy = std::find(actions.begin(), actions.end(),
      open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Smithy))) != actions.end();
  SPIEL_CHECK_TRUE(can_select_smithy);
}

static void TestPlayNonTerminalActionPhase();
static void TestPlayNonTerminalThroneSuppression();
static void TestPlayNonTerminalHandlesShuffle();
static void TestPlayNonTerminalBreaksOnChanceAfterSomeDraw();
static void TestPlayNonTerminalPrefersHigherActions();

int main() {
  TestMarket();
  TestVillage();
  TestSmithy();
  TestFestival();
  TestMoat();
  open_spiel::dominion::RunCellarTests();
  TestDiscardFinishVisibility();
  open_spiel::dominion::RunWorkshopTests();
  TestPlayNonTerminalActionPhase();
  TestPlayNonTerminalThroneSuppression();
  TestPlayNonTerminalHandlesShuffle();
  TestPlayNonTerminalBreaksOnChanceAfterSomeDraw();
  TestPlayNonTerminalPrefersHigherActions();
  TestActionToStringAppendsCardNames();
  open_spiel::dominion::RunChapelTests();
  open_spiel::dominion::RunRemodelTests();
  open_spiel::dominion::RunMilitiaTests();
  open_spiel::dominion::RunWitchTests();
  open_spiel::dominion::RunThroneRoomTests();
  // JSON round-trip checks
  open_spiel::dominion::RunChapelJsonRoundTrip();
  open_spiel::dominion::RunCellarJsonRoundTrip();
  open_spiel::dominion::RunWorkshopJsonRoundTrip();
  open_spiel::dominion::RunRemodelJsonRoundTrip();
  open_spiel::dominion::RunMilitiaJsonRoundTrip();
  open_spiel::dominion::RunThroneRoomJsonRoundTrip();
  open_spiel::dominion::RunWitchJsonRoundTrip();
  return 0;
}
static void TestPlayNonTerminalActionPhase() {
  auto game = LoadGame("dominion");
  auto state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);
  // Give player 0 Village and Smithy.
  ds->player_states_[0].hand_counts_.fill(0);
  ds->player_states_[0].hand_counts_[static_cast<int>(CardName::CARD_Village)] = 1;
  ds->player_states_[0].hand_counts_[static_cast<int>(CardName::CARD_Smithy)] = 1;
  ds->phase_ = Phase::actionPhase;
  ds->actions_ = 2;
  auto la = ds->LegalActions();
  bool has_draw_nonterm = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayNonTerminal()) != la.end();
  SPIEL_CHECK_TRUE(has_draw_nonterm);
  bool has_play_village = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Village))) != la.end();
  SPIEL_CHECK_FALSE(has_play_village);
  bool has_play_smithy = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Smithy))) != la.end();
  SPIEL_CHECK_FALSE(has_play_smithy);
}

static void TestPlayNonTerminalThroneSuppression() {
  auto game = LoadGame("dominion");
  auto state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);
  // Put player into Throne selection with depth 1 and hand Market + Smithy.
  auto& ps = ds->player_states_[0];
  ps.effect_queue.clear();
  ps.hand_counts_.fill(0);
  ps.hand_counts_[static_cast<int>(CardName::CARD_Market)] = 1;
  ps.hand_counts_[static_cast<int>(CardName::CARD_Smithy)] = 1;
  // Install Throne node depth 1.
  ps.effect_queue.push_back(std::unique_ptr<open_spiel::dominion::EffectNode>(new open_spiel::dominion::ThroneRoomEffectNode(1)));
  ds->player_states_[0].pending_choice = open_spiel::dominion::PendingChoice::PlayActionFromHand;
  auto la = ds->LegalActions();
  bool has_draw_nonterm = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayNonTerminal()) != la.end();
  SPIEL_CHECK_FALSE(has_draw_nonterm);
  // Non-terminal Market should be suppressed; terminal Smithy suppressed only when depth >=2 (retained behavior).
  bool has_play_market = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Market))) != la.end();
  SPIEL_CHECK_TRUE(has_play_market);
  bool has_play_smithy = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Smithy))) != la.end();
  SPIEL_CHECK_TRUE(has_play_smithy);
}

static void TestPlayNonTerminalHandlesShuffle() {
  auto game = LoadGame("dominion");
  auto state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);
  auto& ps = ds->player_states_[0];
  ps.hand_counts_.fill(0);
  ps.deck_.clear();
  ps.discard_counts_.fill(0);
  ps.hand_counts_[static_cast<int>(CardName::CARD_Village)] = 1;
  ps.discard_counts_[static_cast<int>(CardName::CARD_Copper)] = 3;
  ds->phase_ = Phase::actionPhase;
  ds->actions_ = 2;
  {
    auto la = ds->LegalActions();
    bool has_dnt = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayNonTerminal()) != la.end();
    SPIEL_CHECK_TRUE(has_dnt);
  }
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayNonTerminal());
  SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, 0), 0);
  SPIEL_CHECK_GE(DominionTestHarness::DeckSize(ds, 0), 2);
  SPIEL_CHECK_GE(DominionTestHarness::Actions(ds), 1);
  // Not at chance node afterwards; loop continued post-shuffle.
  {
    auto la2 = ds->LegalActions();
    bool only_shuffle = (la2.size() == 1 && la2[0] == open_spiel::dominion::ActionIds::Shuffle());
    SPIEL_CHECK_FALSE(only_shuffle);
  }
  // Village should be in play area.
  SPIEL_CHECK_GE(DominionTestHarness::PlayAreaSize(ds), 1);
}

static void TestPlayNonTerminalBreaksOnChanceAfterSomeDraw() {
  auto game = LoadGame("dominion");
  auto state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);
  auto& ps = ds->player_states_[0];
  ps.hand_counts_.fill(0);
  ps.deck_.clear();
  ps.discard_counts_.fill(0);
  // Use Laboratory (+2 Cards, +0 Action) to ensure partial draw before shuffle.
  ps.hand_counts_[static_cast<int>(CardName::CARD_Laboratory)] = 1;
  ps.deck_.push_back(CardName::CARD_Copper);
  ps.discard_counts_[static_cast<int>(CardName::CARD_Copper)] = 3;
  ds->phase_ = Phase::actionPhase;
  ds->actions_ = 2;
  {
    auto la = ds->LegalActions();
    bool has_dnt = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayNonTerminal()) != la.end();
    SPIEL_CHECK_TRUE(has_dnt);
  }
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayNonTerminal());
  {
    auto la2 = ds->LegalActions();
    bool only_shuffle = (la2.size() == 1 && la2[0] == open_spiel::dominion::ActionIds::Shuffle());
    SPIEL_CHECK_FALSE(only_shuffle);
  }
  // Discard should have been emptied into deck by the consumed shuffle.
  SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, 0), 0);
}
static void TestPlayNonTerminalPrefersHigherActions() {
  auto game = LoadGame("dominion");
  auto state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);
  auto& ps = ds->player_states_[0];
  ps.hand_counts_.fill(0);
  ps.deck_.clear();
  ps.discard_counts_.fill(0);
  // Ensure enough capacity: add 10 cards to deck.
  for (int i = 0; i < 10; ++i) ps.deck_.push_back(CardName::CARD_Copper);
  // Put Village (+2 actions, +1 draw) and Laboratory (+1 action, +2 draw) in hand.
  ps.hand_counts_[static_cast<int>(CardName::CARD_Village)] = 1;
  ps.hand_counts_[static_cast<int>(CardName::CARD_Laboratory)] = 1;
  ds->phase_ = Phase::actionPhase;
  ds->actions_ = 2;
  // Composite action should pick the higher-action card first (Village).
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayNonTerminal());
  SPIEL_CHECK_GE(DominionTestHarness::PlayAreaSize(ds), 1);
  CardName first_played = ds->play_area_.empty() ? CardName::CARD_Copper : ds->play_area_.front();
  SPIEL_CHECK_EQ(static_cast<int>(first_played), static_cast<int>(CardName::CARD_Village));
}
