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

  // Trash three cards stepwise, assert effect still pending.
  for (int k = 0; k < 3; ++k) {
    ds->ApplyAction(open_spiel::dominion::ActionIds::TrashHandSelect(0));
    auto la_mid = ds->LegalActions();
    bool still_pending = false;
    for (auto a : la_mid) {
      if (a == open_spiel::dominion::ActionIds::TrashHandSelect(0)) { still_pending = true; break; }
    }
    SPIEL_CHECK_TRUE(still_pending);
  }
  // Trash fourth card: effect resolves fully.
  ds->ApplyAction(open_spiel::dominion::ActionIds::TrashHandSelect(0));

  // After trashing 4, effect ends automatically and no cards were added to discard.
  SPIEL_CHECK_EQ(DiscardSize(ds, 0), discard_before);
  // Hand decreased by 5: played Chapel (-1) and trashed 4 (-4).
  SPIEL_CHECK_EQ(HandSize(ds, 0), hand_before - 5);
}

void RunChapelJsonRoundTrip() {
  using open_spiel::LoadGame;
  using open_spiel::State;
  using open_spiel::Game;

  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  AddCardToHand(ds, 0, CardName::CARD_Estate);
  AddCardToHand(ds, 0, CardName::CARD_Estate);
  AddCardToHand(ds, 0, CardName::CARD_Copper);
  AddCardToHand(ds, 0, CardName::CARD_Copper);
  AddCardToHand(ds, 0, CardName::CARD_Chapel);
  SetPhase(ds, Phase::actionPhase);

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Chapel)));
  // Trash three cards but leave effect pending.
  ds->ApplyAction(open_spiel::dominion::ActionIds::TrashHandSelect(0));
  ds->ApplyAction(open_spiel::dominion::ActionIds::TrashHandSelect(0));
  ds->ApplyAction(open_spiel::dominion::ActionIds::TrashHandSelect(0));

  // JSON round-trip
  std::string json_str = ds->ToJson();
  nlohmann::json j = nlohmann::json::parse(json_str);
  std::unique_ptr<State> state_copy = game->NewInitialState(j);
  auto* ds_copy = dynamic_cast<DominionState*>(state_copy.get());
  SPIEL_CHECK_TRUE(ds_copy != nullptr);
  // Still pending and can trash more or finish.
  auto la = ds_copy->LegalActions();
  bool can_trash = false, can_finish = false;
  for (auto a : la) {
    if (a == open_spiel::dominion::ActionIds::TrashHandSelect(0)) can_trash = true;
    if (a == open_spiel::dominion::ActionIds::TrashHandSelectFinish()) can_finish = true;
  }
  SPIEL_CHECK_TRUE(can_trash);
  SPIEL_CHECK_TRUE(can_finish);

  // String round-trip
  std::string serialized = ds->Serialize();
  std::unique_ptr<State> state_copy2 = game->DeserializeState(serialized);
  auto* ds_copy2 = dynamic_cast<DominionState*>(state_copy2.get());
  SPIEL_CHECK_TRUE(ds_copy2 != nullptr);
  auto la2 = ds_copy2->LegalActions();
  bool can_trash2 = false, can_finish2 = false;
  for (auto a : la2) {
    if (a == open_spiel::dominion::ActionIds::TrashHandSelect(0)) can_trash2 = true;
    if (a == open_spiel::dominion::ActionIds::TrashHandSelectFinish()) can_finish2 = true;
  }
  SPIEL_CHECK_TRUE(can_trash2);
  SPIEL_CHECK_TRUE(can_finish2);
}

} } // namespace open_spiel::dominion
