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
static void AddCardToDeck(DominionState* s, int player, CardName card) {
  s->player_states_[player].deck_.push_back(card);
}
static void SetPhase(DominionState* s, Phase phase) { s->phase_ = phase; }
static int HandSize(DominionState* s, int player) { int c=0; for(int j=0;j<kNumSupplyPiles;++j) c+=s->player_states_[player].hand_counts_[j]; return c; }
static int DiscardSize(DominionState* s, int player) { int c=0; for(int j=0;j<kNumSupplyPiles;++j) c+=s->player_states_[player].discard_counts_[j]; return c; }
static int SupplyCount(DominionState* s, int j) { return s->supply_piles_[j]; }

void RunWitchTests() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  AddCardToHand(ds, 0, CardName::CARD_Witch);
  SetPhase(ds, Phase::actionPhase);
  int hand_before_p0 = HandSize(ds, 0);
  int curse_idx = static_cast<int>(CardName::CARD_Curse);
  int curse_supply_before = SupplyCount(ds, curse_idx);
  int opp_discard_before = DiscardSize(ds, 1);

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Witch)));

  SPIEL_CHECK_EQ(HandSize(ds, 0), hand_before_p0 + 1);
  SPIEL_CHECK_EQ(DiscardSize(ds, 1), opp_discard_before + 1);
  SPIEL_CHECK_EQ(SupplyCount(ds, curse_idx), curse_supply_before - 1);
}

} } // namespace open_spiel::dominion

