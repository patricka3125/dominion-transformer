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
static int HandSize(DominionState* s, int player) { int c=0; for(int j=0;j<kNumSupplyPiles;++j) c+=s->player_states_[player].hand_counts_[j]; return c; }
static int SupplyCount(DominionState* s, int j) { return s->supply_piles_[j]; }

void RunRemodelTests() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  AddCardToHand(ds, 0, CardName::CARD_Estate);
  AddCardToHand(ds, 0, CardName::CARD_Remodel);
  SetPhase(ds, Phase::actionPhase);
  int discard_before = DiscardSize(ds, 0);
  int smithy_idx2 = static_cast<int>(CardName::CARD_Smithy);
  int smithy_pile_before = SupplyCount(ds, smithy_idx2);

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Remodel)));
  ds->ApplyAction(open_spiel::dominion::ActionIds::TrashHandSelect(static_cast<int>(CardName::CARD_Estate)));

  open_spiel::Action gain_smithy = -1;
  for (auto a : ds->LegalActions()) {
    if (a == open_spiel::dominion::ActionIds::GainSelect(smithy_idx2)) {
      gain_smithy = a;
      break;
    }
  }
  SPIEL_CHECK_TRUE(gain_smithy != -1);
  ds->ApplyAction(gain_smithy);

  SPIEL_CHECK_EQ(DiscardSize(ds, 0), discard_before + 1);
  SPIEL_CHECK_EQ(SupplyCount(ds, smithy_idx2), smithy_pile_before - 1);
}

} } // namespace open_spiel::dominion

