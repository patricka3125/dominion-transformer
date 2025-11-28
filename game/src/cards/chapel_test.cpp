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
static int HandSize(DominionState* s, int player) { int c=0; for(int j=0;j<kNumSupplyPiles;++j) c+=s->player_states_[player].hand_counts_[j]; return c; }
static int DiscardSize(DominionState* s, int player) { int c=0; for(int j=0;j<kNumSupplyPiles;++j) c+=s->player_states_[player].discard_counts_[j]; return c; }

void RunChapelTests() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Add two Estates and two Coppers to ensure we can trash 4, then Chapel last.
  AddCardToHand(ds, 0, CardName::CARD_Estate);
  AddCardToHand(ds, 0, CardName::CARD_Estate);
  AddCardToHand(ds, 0, CardName::CARD_Copper);
  AddCardToHand(ds, 0, CardName::CARD_Copper);
  AddCardToHand(ds, 0, CardName::CARD_Chapel);
  SetPhase(ds, Phase::actionPhase);

  int hand_before = HandSize(ds, 0);
  int discard_before = DiscardSize(ds, 0);

  // Play Chapel (last in hand).
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Chapel)));

  // Trash four cards via successive hand selections.
  for (int k = 0; k < 4; ++k) {
    ds->ApplyAction(open_spiel::dominion::ActionIds::TrashHandSelect(0));
  }

  // After trashing 4, effect ends automatically and no cards were added to discard.
  SPIEL_CHECK_EQ(DiscardSize(ds, 0), discard_before);
  // Hand decreased by 5: played Chapel (-1) and trashed 4 (-4).
  SPIEL_CHECK_EQ(HandSize(ds, 0), hand_before - 5);
}

} } // namespace open_spiel::dominion

