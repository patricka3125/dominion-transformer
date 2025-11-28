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
static int Actions(DominionState* s) { return s->actions_; }
static int DiscardSize(DominionState* s, int player) { int c=0; for(int j=0;j<kNumSupplyPiles;++j) c+=s->player_states_[player].discard_counts_[j]; return c; }

void RunCellarTests() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  // Basic discard & draw 2
  {
    std::shared_ptr<const Game> game = LoadGame("dominion");
    std::unique_ptr<State> state = game->NewInitialState();
    auto* ds = dynamic_cast<DominionState*>(state.get());
    SPIEL_CHECK_TRUE(ds != nullptr);

    AddCardToHand(ds, 0, CardName::CARD_Cellar);
    SetPhase(ds, Phase::actionPhase);
    int hand_before = HandSize(ds, 0);
    int actions_before = Actions(ds);
    int discard_before = DiscardSize(ds, 0);

    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Cellar)));
    ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelect(0));
    ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelect(0));
    ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelectFinish());

    SPIEL_CHECK_EQ(HandSize(ds, 0), hand_before - 1);
    SPIEL_CHECK_EQ(Actions(ds), actions_before);
    SPIEL_CHECK_EQ(DiscardSize(ds, 0), discard_before + 2);
  }

  // Zero discard finish
  {
    std::shared_ptr<const Game> game = LoadGame("dominion");
    std::unique_ptr<State> state = game->NewInitialState();
    auto* ds = dynamic_cast<DominionState*>(state.get());
    SPIEL_CHECK_TRUE(ds != nullptr);

    AddCardToHand(ds, 0, CardName::CARD_Cellar);
    SetPhase(ds, Phase::actionPhase);
    int hand_before = HandSize(ds, 0);
    int actions_before = Actions(ds);
    int discard_before = DiscardSize(ds, 0);

    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Cellar)));
    ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelectFinish());

    SPIEL_CHECK_EQ(HandSize(ds, 0), hand_before - 1);
    SPIEL_CHECK_EQ(Actions(ds), actions_before);
    SPIEL_CHECK_EQ(DiscardSize(ds, 0), discard_before);
  }

  // Action strings visibility
  {
    std::shared_ptr<const Game> game = LoadGame("dominion");
    std::unique_ptr<State> state = game->NewInitialState();
    auto* ds = dynamic_cast<DominionState*>(state.get());
    SPIEL_CHECK_TRUE(ds != nullptr);

    AddCardToHand(ds, 0, CardName::CARD_Cellar);
    SetPhase(ds, Phase::actionPhase);
    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Cellar)));
    auto actions = ds->LegalActions();
    bool has_finish = std::find(actions.begin(), actions.end(), open_spiel::dominion::ActionIds::DiscardHandSelectFinish()) != actions.end();
    bool has_select0 = std::find(actions.begin(), actions.end(), open_spiel::dominion::ActionIds::DiscardHandSelect(0)) != actions.end();
    SPIEL_CHECK_TRUE(has_finish);
    SPIEL_CHECK_TRUE(has_select0);
  }

  // One discard then finish
  {
    std::shared_ptr<const Game> game = LoadGame("dominion");
    std::unique_ptr<State> state = game->NewInitialState();
    auto* ds = dynamic_cast<DominionState*>(state.get());
    SPIEL_CHECK_TRUE(ds != nullptr);

    AddCardToHand(ds, 0, CardName::CARD_Cellar);
    SetPhase(ds, Phase::actionPhase);
    int hand_before = HandSize(ds, 0);
    int actions_before = Actions(ds);
    int discard_before = DiscardSize(ds, 0);

    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Cellar)));
    ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelect(0));
    ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelectFinish());

    SPIEL_CHECK_EQ(HandSize(ds, 0), hand_before - 1);
    SPIEL_CHECK_EQ(Actions(ds), actions_before);
    SPIEL_CHECK_EQ(DiscardSize(ds, 0), discard_before + 1);
  }

  // Three discards (robust index selection)
  {
    std::shared_ptr<const Game> game = LoadGame("dominion");
    std::unique_ptr<State> state = game->NewInitialState();
    auto* ds = dynamic_cast<DominionState*>(state.get());
    SPIEL_CHECK_TRUE(ds != nullptr);

    AddCardToHand(ds, 0, CardName::CARD_Cellar);
    SetPhase(ds, Phase::actionPhase);
    int hand_before = HandSize(ds, 0);
    int actions_before = Actions(ds);
    int discard_before = DiscardSize(ds, 0);

    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Cellar)));
    for (int k = 0; k < 3; ++k) {
      auto la = ds->LegalActions();
      open_spiel::Action pick = -1;
      for (auto a : la) {
        std::string s = ds->ActionToString(ds->CurrentPlayer(), a);
        if (s.find("DiscardHandSelect_") == 0) { pick = a; break; }
      }
      SPIEL_CHECK_TRUE(pick != -1);
      ds->ApplyAction(pick);
    }
    ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelectFinish());

    SPIEL_CHECK_EQ(HandSize(ds, 0), hand_before - 1);
    SPIEL_CHECK_EQ(Actions(ds), actions_before);
    SPIEL_CHECK_EQ(DiscardSize(ds, 0), discard_before + 3);
  }
}

void RunCellarJsonRoundTrip() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  AddCardToHand(ds, 0, CardName::CARD_Cellar);
  SetPhase(ds, Phase::actionPhase);
  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Cellar)));
  // Discard two then round-trip before finish.
  ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelect(0));
  ds->ApplyAction(open_spiel::dominion::ActionIds::DiscardHandSelect(0));

  std::string json_str = ds->ToJson();
  nlohmann::json j = nlohmann::json::parse(json_str);
  std::unique_ptr<State> state_copy = game->NewInitialState(j);
  auto* ds_copy = dynamic_cast<DominionState*>(state_copy.get());
  SPIEL_CHECK_TRUE(ds_copy != nullptr);
  auto la = ds_copy->LegalActions();
  bool has_finish = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::DiscardHandSelectFinish()) != la.end();
  bool can_discard = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::DiscardHandSelect(0)) != la.end();
  SPIEL_CHECK_TRUE(has_finish);
  SPIEL_CHECK_TRUE(can_discard);
}

} } // namespace open_spiel::dominion
