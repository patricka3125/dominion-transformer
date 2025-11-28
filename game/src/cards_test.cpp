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
// use qualified open_spiel::dominion::ActionIds below

namespace open_spiel { namespace dominion { void RunChapelTests(); } }
namespace open_spiel { namespace dominion { void RunCellarTests(); } }
namespace open_spiel { namespace dominion { void RunWorkshopTests(); } }
namespace open_spiel { namespace dominion { void RunRemodelTests(); } }
namespace open_spiel { namespace dominion { void RunMilitiaTests(); } }
namespace open_spiel { namespace dominion { void RunWitchTests(); } }
namespace open_spiel { namespace dominion { void RunThroneRoomTests(); } }
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

int main() {
  TestMarket();
  TestVillage();
  TestSmithy();
  TestFestival();
  TestMoat();
  open_spiel::dominion::RunCellarTests();
  TestDiscardFinishVisibility();
  open_spiel::dominion::RunWorkshopTests();
  open_spiel::dominion::RunChapelTests();
  open_spiel::dominion::RunRemodelTests();
  open_spiel::dominion::RunMilitiaTests();
  open_spiel::dominion::RunWitchTests();
  open_spiel::dominion::RunThroneRoomTests();
  return 0;
}
