#include <memory>

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

#include "dominion.hpp"

using open_spiel::LoadGame;
using open_spiel::State;
using open_spiel::Game;

namespace open_spiel { namespace dominion {

// Friend access helper for tests to manipulate private state.
struct DominionTestHarness {
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
  SPIEL_CHECK_EQ(static_cast<int>(returns[0]), 4);
  SPIEL_CHECK_EQ(static_cast<int>(returns[1]), 0);
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
  SPIEL_CHECK_EQ(static_cast<int>(returns[0]), 9);
  SPIEL_CHECK_EQ(static_cast<int>(returns[1]), 0);
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

int main() {
  TestGardensVP();
  TestBasicVPCount();
  TestDeterministicRNGSerialization();
  return 0;
}