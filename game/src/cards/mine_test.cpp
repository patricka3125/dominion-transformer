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
static int DiscardSize(DominionState* s, int player) { int c=0; for(int j=0;j<kNumSupplyPiles;++j) c+=s->player_states_[player].discard_counts_[j]; return c; }
static int SupplyCount(DominionState* s, int j) { return s->supply_piles_[j]; }

void RunMineTests() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Setup: player has Mine and a Copper to trash
  AddCardToHand(ds, 0, CardName::CARD_Mine);
  AddCardToHand(ds, 0, CardName::CARD_Copper);
  SetPhase(ds, Phase::actionPhase);

  int discard_before = DiscardSize(ds, 0);
  int silver_idx = static_cast<int>(CardName::CARD_Silver);
  int village_idx = static_cast<int>(CardName::CARD_Village);
  int silver_pile_before = SupplyCount(ds, silver_idx);

  // Play Mine
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Mine)));
  // Trash Copper from hand
  ds->ApplyAction(open_spiel::dominion::ActionIds::TrashHandSelect(static_cast<int>(CardName::CARD_Copper)));

  // After switching to gain stage: can gain Silver (cost 3), cannot gain non-treasure Village (cost 3)
  auto la = ds->LegalActions();
  bool can_gain_silver = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::GainSelect(silver_idx)) != la.end();
  bool can_gain_village = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::GainSelect(village_idx)) != la.end();
  SPIEL_CHECK_TRUE(can_gain_silver);
  SPIEL_CHECK_FALSE(can_gain_village);

  // Gain Silver and verify state updates
  ds->ApplyAction(open_spiel::dominion::ActionIds::GainSelect(silver_idx));
  SPIEL_CHECK_EQ(DiscardSize(ds, 0), discard_before);
  // Silver should be in hand now
  int hand_silver = ds->player_states_[0].hand_counts_[silver_idx];
  SPIEL_CHECK_EQ(hand_silver, 1);
  SPIEL_CHECK_EQ(SupplyCount(ds, silver_idx), silver_pile_before - 1);
}

} } // namespace open_spiel::dominion
