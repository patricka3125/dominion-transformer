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
static int Actions(DominionState* s) { return s->actions_; }
static int DeckSize(DominionState* s, int player) { return static_cast<int>(s->player_states_[player].deck_.size()); }
static int PlayAreaSize(DominionState* s) { return static_cast<int>(s->play_area_.size()); }

void RunThroneRoomTests() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  // Plays card twice; effect only second time
  {
    std::shared_ptr<const Game> game = LoadGame("dominion");
    std::unique_ptr<State> state = game->NewInitialState();
    auto* ds = dynamic_cast<DominionState*>(state.get());
    SPIEL_CHECK_TRUE(ds != nullptr);

    AddCardToHand(ds, 0, CardName::CARD_Smithy);
    AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
    for (int i = 0; i < 10; ++i) AddCardToDeck(ds, 0, CardName::CARD_Copper);
    SetPhase(ds, Phase::actionPhase);
    int hand_before = HandSize(ds, 0);
    int actions_before = Actions(ds);
    int deck_before = DeckSize(ds, 0);

    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));
    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Smithy)));

    int deck_after = DeckSize(ds, 0);
    int draws = deck_before - deck_after;
    SPIEL_CHECK_EQ(draws, 6);
    SPIEL_CHECK_EQ(HandSize(ds, 0), hand_before - PlayAreaSize(ds) + draws);
    SPIEL_CHECK_EQ(Actions(ds), actions_before - 1);
    auto la = ds->LegalActions();
    bool has_finish = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::ThroneHandSelectFinish()) != la.end();
    SPIEL_CHECK_FALSE(has_finish);
  }

  // Throne into Throne, then Smithy twice
  {
    std::shared_ptr<const Game> game = LoadGame("dominion");
    std::unique_ptr<State> state = game->NewInitialState();
    auto* ds = dynamic_cast<DominionState*>(state.get());
    SPIEL_CHECK_TRUE(ds != nullptr);

    AddCardToHand(ds, 0, CardName::CARD_Smithy);
    AddCardToHand(ds, 0, CardName::CARD_Smithy);
    AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
    AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
    for (int i = 0; i < 30; ++i) AddCardToDeck(ds, 0, CardName::CARD_Copper);
    SetPhase(ds, Phase::actionPhase);

    int hand_before = HandSize(ds, 0);
    int deck_before = DeckSize(ds, 0);

    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));
    {
      ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));
    }
    {
      ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Smithy)));
    }
    {
      ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Smithy)));
    }
    int deck_after_all = DeckSize(ds, 0);
    int total_draws = deck_before - deck_after_all;
    SPIEL_CHECK_EQ(total_draws, 12);
    SPIEL_CHECK_EQ(HandSize(ds, 0), hand_before - PlayAreaSize(ds) + total_draws);
    SPIEL_CHECK_EQ(PlayAreaSize(ds), 4);
    auto la = ds->LegalActions();
    bool has_finish = std::find(
        la.begin(), la.end(), open_spiel::dominion::ActionIds::ThroneHandSelectFinish())
        != la.end();
    SPIEL_CHECK_FALSE(has_finish);
  }

  // Allows finish with no selection
  {
    std::shared_ptr<const Game> game = LoadGame("dominion");
    std::unique_ptr<State> state = game->NewInitialState();
    auto* ds = dynamic_cast<DominionState*>(state.get());
    SPIEL_CHECK_TRUE(ds != nullptr);

    AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
    SetPhase(ds, Phase::actionPhase);
    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));
    auto la = ds->LegalActions();
    bool has_finish = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::ThroneHandSelectFinish()) != la.end();
    SPIEL_CHECK_TRUE(has_finish);
  }

  // Ignores ascending constraint in chain (use non-draw ACTION)
  {
    std::shared_ptr<const Game> game = LoadGame("dominion");
    std::unique_ptr<State> state = game->NewInitialState();
    auto* ds = dynamic_cast<DominionState*>(state.get());
    SPIEL_CHECK_TRUE(ds != nullptr);

    AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
    AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
    AddCardToHand(ds, 0, CardName::CARD_Festival);
    SetPhase(ds, Phase::actionPhase);

    ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));
    {
      ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));
    }
    auto actions = ds->LegalActions();
    bool can_select_festival = std::find(actions.begin(), actions.end(),
        open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Festival))) != actions.end();
    SPIEL_CHECK_TRUE(can_select_festival);
  }
}

void RunThroneRoomJsonRoundTrip() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  AddCardToHand(ds, 0, CardName::CARD_ThroneRoom);
  AddCardToHand(ds, 0, CardName::CARD_Smithy);
  SetPhase(ds, Phase::actionPhase);

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_ThroneRoom)));
  // Round-trip while selection pending.
  std::string json_str = ds->ToJson();
  nlohmann::json j = nlohmann::json::parse(json_str);
  std::unique_ptr<State> state_copy = game->NewInitialState(j);
  auto* ds_copy = dynamic_cast<DominionState*>(state_copy.get());
  SPIEL_CHECK_TRUE(ds_copy != nullptr);
  auto actions = ds_copy->LegalActions();
  bool can_select_action = false;
  for (auto a : actions) {
    std::string s = ds_copy->ActionToString(ds_copy->CurrentPlayer(), a);
    if (s.find("PlayHandIndex_") == 0) { can_select_action = true; break; }
  }
  SPIEL_CHECK_TRUE(can_select_action);
}

} } // namespace open_spiel::dominion
