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

void RunMilitiaTests() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  AddCardToHand(ds, 0, CardName::CARD_Militia);
  SetPhase(ds, Phase::actionPhase);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Militia)));

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

  SPIEL_CHECK_EQ(ds->CurrentPlayer(), 0);
  SPIEL_CHECK_EQ(HandSize(ds, 1), 3);
}

void RunMilitiaJsonRoundTrip() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  AddCardToHand(ds, 0, CardName::CARD_Militia);
  SetPhase(ds, Phase::actionPhase);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Militia)));
  // Opponent discards one card; still pending to reach 3.
  ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelect(0));

  std::string json_str = ds->ToJson();
  nlohmann::json j = nlohmann::json::parse(json_str);
  std::unique_ptr<State> state_copy = game->NewInitialState(j);
  auto* ds_copy = dynamic_cast<DominionState*>(state_copy.get());
  SPIEL_CHECK_TRUE(ds_copy != nullptr);
  // Current player remains opponent; finish still not legal.
  SPIEL_CHECK_EQ(ds_copy->CurrentPlayer(), 1);
  auto la = ds_copy->LegalActions();
  bool has_finish = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::DiscardHandSelectFinish()) != la.end();
  SPIEL_CHECK_FALSE(has_finish);
}

} } // namespace open_spiel::dominion
