#include <memory>

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

#include "dominion.hpp"
#include "actions.hpp"

using open_spiel::LoadGame;
using open_spiel::State;
using open_spiel::Game;

namespace open_spiel { namespace dominion {

// Friend access helper for tests to manipulate private state.
struct DominionTestHarness {
  // Accessors for verification of private fields.
  static const std::array<int, kNumSupplyPiles>& SupplyCounts(DominionState* s) {
    return s->supply_piles_;
  }
  static CardName SupplyType(DominionState* s, int idx) {
    (void)s; return static_cast<CardName>(idx);
  }
  static int DeckSize(DominionState* s, int player) { return static_cast<int>(s->player_states_[player].deck_.size()); }
  static int HandSize(DominionState* s, int player) { return static_cast<int>(s->player_states_[player].hand_.size()); }
  static int DiscardSize(DominionState* s, int player) { return static_cast<int>(s->player_states_[player].discard_.size()); }
  static bool HasObsState(DominionState* s, int player) { return s->player_states_[player].obs_state != nullptr; }
  static const ObservationState* Obs(DominionState* s, int player) { return s->player_states_[player].obs_state.get(); }
  static int CurrentPlayer(DominionState* s) { return s->current_player_; }
  static int Actions(DominionState* s) { return s->actions_; }
  static int Buys(DominionState* s) { return s->buys_; }
  static int Coins(DominionState* s) { return s->coins_; }
  static Phase PhaseVal(DominionState* s) { return s->phase_; }
  static int TurnNumber(DominionState* s) { return s->turn_number_; }
  static void ResetPlayer(DominionState* s, int player) {
    s->player_states_[player].deck_.clear();
    s->player_states_[player].hand_.clear();
    s->player_states_[player].discard_.clear();
  }
  static void AddCardToDeck(DominionState* s, int player, CardName card) {
    s->player_states_[player].deck_.push_back(card);
  }
  static void AddCardToHand(DominionState* s, int player, CardName card) {
    s->player_states_[player].hand_.push_back(card);
  }
  static void AddCardToDiscard(DominionState* s, int player, CardName card) {
    s->player_states_[player].discard_.push_back(card);
  }
  static void SetProvinceEmpty(DominionState* s) {
    s->supply_piles_[5] = 0; // Province index
  }
  // Returns a copy of the current hand for verification.
  static std::vector<CardName> Hand(DominionState* s, int player) {
    return s->player_states_[player].hand_;
  }
};

} }

using open_spiel::dominion::DominionState;
using open_spiel::dominion::CardName;
using open_spiel::dominion::DominionTestHarness;
using open_spiel::dominion::ObservationState;
using open_spiel::dominion::kNumSupplyPiles;
using open_spiel::dominion::kNumPlayers;
using open_spiel::dominion::Phase;
// ActionIds is a namespace; refer to it with a qualified name.

static void TestEndBuySwitchesPlayerAndTurnIncrements();
static void TestAutoEndOnLastBuy();

// Gardens: 1 VP per Gardens for every 10 total cards (deck+discard+hand).
static void TestGardensVP() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Terminal condition.
  DominionTestHarness::SetProvinceEmpty(ds);

  // Reset both players to remove base Estates VP and create controlled totals.
  DominionTestHarness::ResetPlayer(ds, 0);
  DominionTestHarness::ResetPlayer(ds, 1);

  // Player 0: total 20 cards, 2 Gardens => 2 * (20/10) = 4 VP.
  for (int i = 0; i < 18; ++i) DominionTestHarness::AddCardToDeck(ds, 0, CardName::CARD_Copper);
  DominionTestHarness::AddCardToDeck(ds, 0, CardName::CARD_Gardens);
  DominionTestHarness::AddCardToDeck(ds, 0, CardName::CARD_Gardens);

  // Player 1: empty -> 0 VP.

  auto returns = ds->Returns();
  SPIEL_CHECK_EQ(static_cast<int>(returns[0]), 1);
  SPIEL_CHECK_EQ(static_cast<int>(returns[1]), -1);
}

// Basic VP: Estate (1), Duchy (3), Province (6), Curse (-1) => total 9 VP.
static void TestBasicVPCount() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Terminal condition.
  DominionTestHarness::SetProvinceEmpty(ds);

  // Reset player 0; player 1 empty.
  DominionTestHarness::ResetPlayer(ds, 0);
  DominionTestHarness::ResetPlayer(ds, 1);

  // Add scoring cards to player 0.
  DominionTestHarness::AddCardToDeck(ds, 0, CardName::CARD_Estate);
  DominionTestHarness::AddCardToDeck(ds, 0, CardName::CARD_Duchy);
  DominionTestHarness::AddCardToDeck(ds, 0, CardName::CARD_Province);
  DominionTestHarness::AddCardToDeck(ds, 0, CardName::CARD_Curse);

  auto returns = ds->Returns();
  SPIEL_CHECK_EQ(static_cast<int>(returns[0]), 1);
  SPIEL_CHECK_EQ(static_cast<int>(returns[1]), -1);
}

// Tie-breaker and draw rules:
// - If VP ties and second player was last to go, it's a draw (0,0).
// - If VP ties and first player was last to go, player 2 wins (-1, +1).
static void TestTieBreakerAndDrawRules() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  auto end_turn = [&](DominionState* s) {
    s->ApplyAction(open_spiel::dominion::ActionIds::EndActions());
    s->ApplyAction(open_spiel::dominion::ActionIds::EndBuy());
  };

  // Equal VP for both players.
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Estate);
  DominionTestHarness::AddCardToDiscard(ds, 1, CardName::CARD_Estate);

  // Make player 1 the last to go.
  end_turn(ds);  // Player 0 finishes
  end_turn(ds);  // Player 1 finishes
  DominionTestHarness::SetProvinceEmpty(ds);
  SPIEL_CHECK_TRUE(ds->IsTerminal());
  auto returns_draw = ds->Returns();
  SPIEL_CHECK_EQ(static_cast<int>(returns_draw[0]), 0);
  SPIEL_CHECK_EQ(static_cast<int>(returns_draw[1]), 0);

  // Reset and make player 0 the last to go; tie => player 2 wins.
  state = game->NewInitialState();
  ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Estate);
  DominionTestHarness::AddCardToDiscard(ds, 1, CardName::CARD_Estate);
  end_turn(ds);  // Player 0 finishes
  DominionTestHarness::SetProvinceEmpty(ds);
  SPIEL_CHECK_TRUE(ds->IsTerminal());
  auto returns_p2win = ds->Returns();
  SPIEL_CHECK_EQ(static_cast<int>(returns_p2win[0]), -1);
  SPIEL_CHECK_EQ(static_cast<int>(returns_p2win[1]), 1);
}

// Determinism and RNG serialization: with the same rng_seed and a saved
// RNG state restored on a fresh game, identical chance outcomes should occur.
static void TestDeterministicRNGSerialization() {
  // Single game determinism: save RNG, perform shuffle, restore RNG and repeat.
  std::shared_ptr<const Game> game = LoadGame("dominion(rng_seed=42)");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Save RNG state prior to performing any new random operations.
  std::string rng_state = game->GetRNGState();

  // Forced shuffle scenario: empty deck, known discard order.
  DominionTestHarness::ResetPlayer(ds, 0);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Copper);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Silver);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Gold);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Estate);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Duchy);

  ds->DrawCardsFor(0, 5);
  auto hand_first = DominionTestHarness::Hand(ds, 0);
  SPIEL_CHECK_EQ(static_cast<int>(hand_first.size()), 5);

  // Reset to identical scenario and restore RNG state; draw order must match.
  game->SetRNGState(rng_state);
  DominionTestHarness::ResetPlayer(ds, 0);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Copper);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Silver);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Gold);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Estate);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Duchy);
  ds->DrawCardsFor(0, 5);
  auto hand_second = DominionTestHarness::Hand(ds, 0);
  SPIEL_CHECK_EQ(static_cast<int>(hand_second.size()), 5);
  for (int i = 0; i < 5; ++i) {
    SPIEL_CHECK_EQ(static_cast<int>(hand_first[i]), static_cast<int>(hand_second[i]));
  }

  // Sanity check: a different seed should produce a different order.
  std::shared_ptr<const Game> game_c = LoadGame("dominion(rng_seed=7)");
  std::unique_ptr<State> state_c = game_c->NewInitialState();
  auto* ds_c = dynamic_cast<DominionState*>(state_c.get());
  SPIEL_CHECK_TRUE(ds_c != nullptr);
  DominionTestHarness::ResetPlayer(ds_c, 0);
  DominionTestHarness::AddCardToDiscard(ds_c, 0, CardName::CARD_Copper);
  DominionTestHarness::AddCardToDiscard(ds_c, 0, CardName::CARD_Silver);
  DominionTestHarness::AddCardToDiscard(ds_c, 0, CardName::CARD_Gold);
  DominionTestHarness::AddCardToDiscard(ds_c, 0, CardName::CARD_Estate);
  DominionTestHarness::AddCardToDiscard(ds_c, 0, CardName::CARD_Duchy);
  ds_c->DrawCardsFor(0, 5);
  auto hand_c = DominionTestHarness::Hand(ds_c, 0);
  SPIEL_CHECK_EQ(static_cast<int>(hand_c.size()), 5);
  bool any_diff = false;
  for (int i = 0; i < 5; ++i) any_diff |= (hand_first[i] != hand_c[i]);
  SPIEL_CHECK_TRUE(any_diff);
}

// Verify initial constructor invariants for DominionState.
static void TestInitialConstructorState() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Supply types and counts.
  const auto& counts = DominionTestHarness::SupplyCounts(ds);
  SPIEL_CHECK_EQ(static_cast<int>(counts.size()), kNumSupplyPiles);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Copper)], 60);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Silver)], 40);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Gold)], 30);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Estate)], 8);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Duchy)], 8);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Province)], 8);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Curse)], 10);
  // Selected kingdom piles should be initialized to 10; others 0.
  auto is_selected = [](CardName cn){
    switch (cn) {
      case CardName::CARD_Cellar:
      case CardName::CARD_Market:
      case CardName::CARD_Militia:
      case CardName::CARD_Mine:
      case CardName::CARD_Moat:
      case CardName::CARD_Remodel:
      case CardName::CARD_Smithy:
      case CardName::CARD_Village:
      case CardName::CARD_Workshop:
      case CardName::CARD_Festival:
        return true;
      default: return false;
    }
  };
  for (int i = 7; i < kNumSupplyPiles; ++i) {
    CardName cn = static_cast<CardName>(i);
    if (is_selected(cn)) {
      SPIEL_CHECK_EQ(counts[i], 10);
    } else {
      SPIEL_CHECK_EQ(counts[i], 0);
    }
  }

  // Player observation states exist and reflect deck/discard sizes via pointers.
  for (int p = 0; p < kNumPlayers; ++p) {
    SPIEL_CHECK_TRUE(DominionTestHarness::HasObsState(ds, p));
    const ObservationState* obs = DominionTestHarness::Obs(ds, p);
    // Deck and discard sizes per ObservationState should match actual vectors.
    int deck_size = static_cast<int>(obs->player_deck.size());
    SPIEL_CHECK_EQ(deck_size, DominionTestHarness::DeckSize(ds, p));
    int discard_size = static_cast<int>(obs->player_discard.size());
    SPIEL_CHECK_EQ(discard_size, DominionTestHarness::DiscardSize(ds, p));
  }

  // Initial decks and hands: 10-card deck then draw 5 => hand=5, deck=5, discard=0.
  for (int p = 0; p < kNumPlayers; ++p) {
    SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, p), 5);
    SPIEL_CHECK_EQ(DominionTestHarness::DeckSize(ds, p), 5);
    SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, p), 0);
  }

  // Turn/phase and counters.
  SPIEL_CHECK_EQ(DominionTestHarness::CurrentPlayer(ds), 0);
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Buys(ds), 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Coins(ds), 0);
  SPIEL_CHECK_EQ(static_cast<int>(DominionTestHarness::PhaseVal(ds)), static_cast<int>(open_spiel::dominion::Phase::actionPhase));
}

int main() {
  TestEndBuySwitchesPlayerAndTurnIncrements();
  TestAutoEndOnLastBuy();
  TestGardensVP();
  TestBasicVPCount();
  TestTieBreakerAndDrawRules();
  TestDeterministicRNGSerialization();
  TestInitialConstructorState();
  return 0;
}
// Verify EndBuy switches to next player and increments turn number (1-based).
static void TestEndBuySwitchesPlayerAndTurnIncrements() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  SPIEL_CHECK_EQ(DominionTestHarness::CurrentPlayer(ds), 0);
  SPIEL_CHECK_EQ(DominionTestHarness::TurnNumber(ds), 1);

  ds->ApplyAction(open_spiel::dominion::ActionIds::EndActions());
  ds->ApplyAction(open_spiel::dominion::ActionIds::EndBuy());

  SPIEL_CHECK_EQ(DominionTestHarness::CurrentPlayer(ds), 1);
  SPIEL_CHECK_EQ(DominionTestHarness::TurnNumber(ds), 2);
}

// Verify that spending the last buy auto-ends the turn.
static void TestAutoEndOnLastBuy() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Move to buy phase.
  ds->ApplyAction(open_spiel::dominion::ActionIds::EndActions());
  // Ensure Copper (cost 0) is buyable with 1 buy.
  int turn_before = DominionTestHarness::TurnNumber(ds);
  int player_before = DominionTestHarness::CurrentPlayer(ds);
  int copper_idx = static_cast<int>(open_spiel::dominion::CardName::CARD_Copper);
  ds->ApplyAction(open_spiel::dominion::ActionIds::BuyFromSupply(copper_idx));
  // After spending last buy, turn should auto-end.
  SPIEL_CHECK_EQ(DominionTestHarness::TurnNumber(ds), turn_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::CurrentPlayer(ds), 1 - player_before);
  SPIEL_CHECK_EQ(static_cast<int>(DominionTestHarness::PhaseVal(ds)), static_cast<int>(open_spiel::dominion::Phase::actionPhase));
}