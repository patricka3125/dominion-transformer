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

void RunWorkshopTests() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  AddCardToHand(ds, 0, CardName::CARD_Workshop);
  SetPhase(ds, Phase::actionPhase);
  int discard_before = DiscardSize(ds, 0);
  int smithy_idx = static_cast<int>(CardName::CARD_Smithy);
  int smithy_pile_before = SupplyCount(ds, smithy_idx);

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Workshop)));

  open_spiel::Action gain_smithy = -1;
  for (auto a : ds->LegalActions()) {
    if (a == open_spiel::dominion::ActionIds::GainSelect(smithy_idx)) {
      gain_smithy = a;
      break;
    }
  }
  SPIEL_CHECK_TRUE(gain_smithy != -1);
  ds->ApplyAction(gain_smithy);

  SPIEL_CHECK_EQ(DiscardSize(ds, 0), discard_before + 1);
  SPIEL_CHECK_EQ(SupplyCount(ds, smithy_idx), smithy_pile_before - 1);
}

void RunWorkshopJsonRoundTrip() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  AddCardToHand(ds, 0, CardName::CARD_Workshop);
  SetPhase(ds, Phase::actionPhase);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Workshop)));

  std::string json_str = ds->ToJson();
  nlohmann::json j = nlohmann::json::parse(json_str);
  std::unique_ptr<State> state_copy = game->NewInitialState(j);
  auto* ds_copy = dynamic_cast<DominionState*>(state_copy.get());
  SPIEL_CHECK_TRUE(ds_copy != nullptr);
  auto la = ds_copy->LegalActions();
  int smithy_idx = static_cast<int>(CardName::CARD_Smithy);
  bool can_gain_smithy = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::GainSelect(smithy_idx)) != la.end();
  SPIEL_CHECK_TRUE(can_gain_smithy);
}

} } // namespace open_spiel::dominion
