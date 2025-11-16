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

int main() {
  TestGardensVP();
  TestBasicVPCount();
  return 0;
}