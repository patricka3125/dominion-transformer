#include <memory>

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

#include "dominion.hpp"
#include "actions.hpp"
#include "effects.hpp"

using open_spiel::LoadGame;
using open_spiel::State;
using open_spiel::Game;

namespace open_spiel { namespace dominion {

// Friend access helper for tests to manipulate private state.
struct DominionTestHarness {
  // Accessors for verification of private fields.
  static const std::array<int, kNumSupplyPiles>& SupplyCounts(DominionState* s) {
    return s->supply_piles_;
  }
  static CardName SupplyType(DominionState* s, int idx) {
    (void)s; return static_cast<CardName>(idx);
  }
  static int DeckSize(DominionState* s, int player) { return static_cast<int>(s->player_states_[player].deck_.size()); }
  static int HandSize(DominionState* s, int player) {
    int cnt = 0; for (int j=0;j<kNumSupplyPiles;++j) cnt += s->player_states_[player].hand_counts_[j];
    return cnt;
  }
  static int DiscardSize(DominionState* s, int player) {
    int cnt = 0; for (int j=0;j<kNumSupplyPiles;++j) cnt += s->player_states_[player].discard_counts_[j];
    return cnt;
  }
  
  static bool HasObsState(DominionState* s, int player) { return s->player_states_[player].obs_state != nullptr; }
  static const ObservationState* Obs(DominionState* s, int player) { return s->player_states_[player].obs_state.get(); }
  static int CurrentPlayer(DominionState* s) { return s->current_player_; }
  static int Actions(DominionState* s) { return s->actions_; }
  static int Buys(DominionState* s) { return s->buys_; }
  static int Coins(DominionState* s) { return s->coins_; }
  static Phase PhaseVal(DominionState* s) { return s->phase_; }
  static int TurnNumber(DominionState* s) { return s->turn_number_; }
  static void ResetPlayer(DominionState* s, int player) {
    s->player_states_[player].deck_.clear();
    s->player_states_[player].hand_counts_.fill(0);
    s->player_states_[player].discard_counts_.fill(0);
  }
  static void AddCardToDeck(DominionState* s, int player, CardName card) {
    s->player_states_[player].deck_.push_back(card);
  }
  static void AddCardToHand(DominionState* s, int player, CardName card) {
    int idx = static_cast<int>(card);
    if (idx >= 0 && idx < kNumSupplyPiles) s->player_states_[player].hand_counts_[idx] += 1;
  }
  static void AddCardToDiscard(DominionState* s, int player, CardName card) {
    int idx = static_cast<int>(card);
    if (idx >= 0 && idx < kNumSupplyPiles) s->player_states_[player].discard_counts_[idx] += 1;
  }
  static void SetProvinceEmpty(DominionState* s) {
    s->supply_piles_[5] = 0; // Province index
  }
  // Returns a copy of the current hand for verification.
  static std::array<int, kNumSupplyPiles> Hand(DominionState* s, int player) {
    return s->player_states_[player].hand_counts_;
  }
};

} }

using open_spiel::dominion::DominionState;
using open_spiel::dominion::CardName;
using open_spiel::dominion::DominionTestHarness;
using open_spiel::dominion::ObservationState;
using open_spiel::dominion::kNumSupplyPiles;
using open_spiel::dominion::kNumPlayers;
using open_spiel::dominion::Phase;
// ActionIds is a namespace; refer to it with a qualified name.

static void TestEndBuySwitchesPlayerAndTurnIncrements();
static void TestAutoEndOnLastBuy();
static void TestAutoAdvanceOnZeroActions();
static void TestDominionStateJsonRoundTrip();
static void TestDominionStateSerializeDeserialize();
static void TestEffectQueueJsonRoundTrip();
static void TestThroneRoomChainJsonRoundTrip();
static void TestEffectQueueSerializeDeserialize();

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
  SPIEL_CHECK_EQ(static_cast<int>(returns[0]), 1);
  SPIEL_CHECK_EQ(static_cast<int>(returns[1]), -1);
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
  SPIEL_CHECK_EQ(static_cast<int>(returns[0]), 1);
  SPIEL_CHECK_EQ(static_cast<int>(returns[1]), -1);
}

// Tie-breaker and draw rules:
// - If VP ties and second player was last to go, it's a draw (0,0).
// - If VP ties and first player was last to go, player 2 wins (-1, +1).
static void TestTieBreakerAndDrawRules() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  auto end_turn = [&](DominionState* s) {
    s->ApplyAction(open_spiel::dominion::ActionIds::EndActions());
    s->ApplyAction(open_spiel::dominion::ActionIds::EndBuy());
  };

  // Equal VP for both players.
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Estate);
  DominionTestHarness::AddCardToDiscard(ds, 1, CardName::CARD_Estate);

  // Make player 1 the last to go.
  end_turn(ds);  // Player 0 finishes
  end_turn(ds);  // Player 1 finishes
  DominionTestHarness::SetProvinceEmpty(ds);
  SPIEL_CHECK_TRUE(ds->IsTerminal());
  auto returns_draw = ds->Returns();
  SPIEL_CHECK_EQ(static_cast<int>(returns_draw[0]), 0);
  SPIEL_CHECK_EQ(static_cast<int>(returns_draw[1]), 0);

  // Reset and make player 0 the last to go; tie => player 2 wins.
  state = game->NewInitialState();
  ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);
  DominionTestHarness::AddCardToDiscard(ds, 0, CardName::CARD_Estate);
  DominionTestHarness::AddCardToDiscard(ds, 1, CardName::CARD_Estate);
  end_turn(ds);  // Player 0 finishes
  DominionTestHarness::SetProvinceEmpty(ds);
  SPIEL_CHECK_TRUE(ds->IsTerminal());
  auto returns_p2win = ds->Returns();
  SPIEL_CHECK_EQ(static_cast<int>(returns_p2win[0]), -1);
  SPIEL_CHECK_EQ(static_cast<int>(returns_p2win[1]), 1);
}

// Buy phase should not offer PlayHandIndex for basic treasures.
static void TestBuyPhaseNoBasicTreasurePlayOptions() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Controlled hand: Copper, Silver, Gold.
  DominionTestHarness::ResetPlayer(ds, 0);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Copper);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Silver);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Gold);

  auto la = ds->LegalActions();
  int copper_idx = static_cast<int>(CardName::CARD_Copper);
  int silver_idx = static_cast<int>(CardName::CARD_Silver);
  int gold_idx = static_cast<int>(CardName::CARD_Gold);
  bool has_play_copper = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayHandIndex(copper_idx)) != la.end();
  bool has_play_silver = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayHandIndex(silver_idx)) != la.end();
  bool has_play_gold = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::PlayHandIndex(gold_idx)) != la.end();
  SPIEL_CHECK_FALSE(has_play_copper);
  SPIEL_CHECK_FALSE(has_play_silver);
  SPIEL_CHECK_FALSE(has_play_gold);
}

// BuyFromSupply gates by effective coins: coins + basic treasure values in hand.
static void TestBuyPhaseEffectiveCoinsGateBuys() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Hand: Gold (3), Copper (1) => effective coins 4.
  DominionTestHarness::ResetPlayer(ds, 0);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Gold);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Copper);

  auto la = ds->LegalActions();
  int smithy_idx = static_cast<int>(CardName::CARD_Smithy);   // cost 4
  int festival_idx = static_cast<int>(CardName::CARD_Festival); // cost 5
  bool can_buy_smithy = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::BuyFromSupply(smithy_idx)) != la.end();
  bool can_buy_festival = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::BuyFromSupply(festival_idx)) != la.end();
  SPIEL_CHECK_TRUE(can_buy_smithy);
  SPIEL_CHECK_FALSE(can_buy_festival);
}

// Applying BuyFromSupply should auto-play all basic treasures first.
static void TestBuyFromSupplyAutoPlaysBasicTreasures() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  DominionTestHarness::ResetPlayer(ds, 0);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Copper);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Silver);

  int discard_before = DominionTestHarness::DiscardSize(ds, 0);
  int silver_idx = static_cast<int>(CardName::CARD_Silver);
  int silver_supply_before = DominionTestHarness::SupplyCounts(ds)[silver_idx];

  ds->ApplyAction(open_spiel::dominion::ActionIds::BuyFromSupply(silver_idx));

  SPIEL_CHECK_EQ(DominionTestHarness::SupplyCounts(ds)[silver_idx], silver_supply_before - 1);
  SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, 0), discard_before + 3);
}


// Verify initial constructor invariants for DominionState.
static void TestInitialConstructorState() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Supply types and counts.
  const auto& counts = DominionTestHarness::SupplyCounts(ds);
  SPIEL_CHECK_EQ(static_cast<int>(counts.size()), kNumSupplyPiles);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Copper)], 60);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Silver)], 40);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Gold)], 30);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Estate)], 8);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Duchy)], 8);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Province)], 8);
  SPIEL_CHECK_EQ(counts[static_cast<int>(CardName::CARD_Curse)], 10);
  // Kingdom composition is implementation-defined; base supply checks above are sufficient.

  // Player observation states exist and reflect deck/discard sizes via pointers.
  for (int p = 0; p < kNumPlayers; ++p) {
    SPIEL_CHECK_TRUE(DominionTestHarness::HasObsState(ds, p));
    const ObservationState* obs = DominionTestHarness::Obs(ds, p);
    // Deck and discard sizes per ObservationState should match actual vectors.
    int deck_size = static_cast<int>(obs->player_deck.size());
    SPIEL_CHECK_EQ(deck_size, DominionTestHarness::DeckSize(ds, p));
    int discard_size = 0; for (int j=0;j<kNumSupplyPiles;++j) discard_size += obs->player_discard_counts[j];
    SPIEL_CHECK_EQ(discard_size, DominionTestHarness::DiscardSize(ds, p));
  }

  // Initial decks and hands: 10-card deck then draw 5 => hand=5, deck=5, discard=0.
  for (int p = 0; p < kNumPlayers; ++p) {
    SPIEL_CHECK_EQ(DominionTestHarness::HandSize(ds, p), 5);
    SPIEL_CHECK_EQ(DominionTestHarness::DeckSize(ds, p), 5);
    SPIEL_CHECK_EQ(DominionTestHarness::DiscardSize(ds, p), 0);
  }

  // Turn/phase and counters.
  SPIEL_CHECK_EQ(DominionTestHarness::CurrentPlayer(ds), 0);
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Buys(ds), 1);
  SPIEL_CHECK_EQ(DominionTestHarness::Coins(ds), 0);
  // With auto-advance, initial state begins in buy phase due to no action cards.
  SPIEL_CHECK_EQ(static_cast<int>(DominionTestHarness::PhaseVal(ds)), static_cast<int>(open_spiel::dominion::Phase::buyPhase));
}

int main() {
  TestEndBuySwitchesPlayerAndTurnIncrements();
  TestAutoEndOnLastBuy();
  TestAutoAdvanceOnZeroActions();
  TestGardensVP();
  TestBasicVPCount();
  TestTieBreakerAndDrawRules();
  TestInitialConstructorState();
  TestBuyPhaseNoBasicTreasurePlayOptions();
  TestBuyPhaseEffectiveCoinsGateBuys();
  TestBuyFromSupplyAutoPlaysBasicTreasures();
  TestDominionStateJsonRoundTrip();
  TestDominionStateSerializeDeserialize();
  TestEffectQueueJsonRoundTrip();
  TestThroneRoomChainJsonRoundTrip();
  TestEffectQueueSerializeDeserialize();
  return 0;
}
// Playing an action that consumes the last action should auto-advance to buy phase.
static void TestAutoAdvanceOnZeroActions() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Controlled hand: clear then add a single Smithy (grants +3 cards, +0 actions).
  DominionTestHarness::ResetPlayer(ds, 0);
  DominionTestHarness::AddCardToHand(ds, 0, CardName::CARD_Smithy);
  // Ensure we have exactly 1 action.
  ds->actions_ = 1;
  // Must be in action phase to play the card.
  ds->phase_ = Phase::actionPhase;

  ds->ApplyAction(open_spiel::dominion::ActionIds::PlayHandIndex(static_cast<int>(CardName::CARD_Smithy)));
  // After playing Smithy, actions drop to 0 and auto-advance should switch to buy phase.
  SPIEL_CHECK_EQ(static_cast<int>(DominionTestHarness::PhaseVal(ds)), static_cast<int>(open_spiel::dominion::Phase::buyPhase));
  // In buy phase, EndActions should no longer be a legal action.
  auto la = ds->LegalActions();
  bool has_end_actions = std::find(la.begin(), la.end(), open_spiel::dominion::ActionIds::EndActions()) != la.end();
  SPIEL_CHECK_FALSE(has_end_actions);
}
// Verify EndBuy switches to next player and increments turn number (1-based).
static void TestEndBuySwitchesPlayerAndTurnIncrements() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  SPIEL_CHECK_EQ(DominionTestHarness::CurrentPlayer(ds), 0);
  SPIEL_CHECK_EQ(DominionTestHarness::TurnNumber(ds), 1);

  ds->ApplyAction(open_spiel::dominion::ActionIds::EndActions());
  ds->ApplyAction(open_spiel::dominion::ActionIds::EndBuy());

  SPIEL_CHECK_EQ(DominionTestHarness::CurrentPlayer(ds), 1);
  SPIEL_CHECK_EQ(DominionTestHarness::TurnNumber(ds), 2);
}

// Verify that spending the last buy auto-ends the turn.
static void TestAutoEndOnLastBuy() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  // Move to buy phase.
  ds->ApplyAction(open_spiel::dominion::ActionIds::EndActions());
  // Ensure Copper (cost 0) is buyable with 1 buy.
  int turn_before = DominionTestHarness::TurnNumber(ds);
  int player_before = DominionTestHarness::CurrentPlayer(ds);
  int copper_idx = static_cast<int>(open_spiel::dominion::CardName::CARD_Copper);
  ds->ApplyAction(open_spiel::dominion::ActionIds::BuyFromSupply(copper_idx));
  // After spending last buy, turn should auto-end.
  SPIEL_CHECK_EQ(DominionTestHarness::TurnNumber(ds), turn_before + 1);
  SPIEL_CHECK_EQ(DominionTestHarness::CurrentPlayer(ds), 1 - player_before);
  SPIEL_CHECK_EQ(static_cast<int>(DominionTestHarness::PhaseVal(ds)), static_cast<int>(open_spiel::dominion::Phase::buyPhase));
}

// Verify that ToJson and NewInitialState(json) round-trip to equivalent states.
static void TestDominionStateJsonRoundTrip() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  std::string json_str = ds->ToJson();
  nlohmann::json j = nlohmann::json::parse(json_str);
  std::unique_ptr<State> state_copy = game->NewInitialState(j);
  auto* ds_copy = dynamic_cast<DominionState*>(state_copy.get());
  SPIEL_CHECK_TRUE(ds_copy != nullptr);

  SPIEL_CHECK_EQ(ds->ToString(), ds_copy->ToString());
  SPIEL_CHECK_EQ(DominionTestHarness::CurrentPlayer(ds), DominionTestHarness::CurrentPlayer(ds_copy));
  SPIEL_CHECK_EQ(DominionTestHarness::Actions(ds), DominionTestHarness::Actions(ds_copy));
  SPIEL_CHECK_EQ(DominionTestHarness::Buys(ds), DominionTestHarness::Buys(ds_copy));
  SPIEL_CHECK_EQ(DominionTestHarness::Coins(ds), DominionTestHarness::Coins(ds_copy));
}

// Verify that Serialize and DeserializeState round-trip equivalently.
static void TestDominionStateSerializeDeserialize() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  std::string serialized = ds->Serialize();
  std::unique_ptr<State> state_copy = game->DeserializeState(serialized);
  auto* ds_copy = dynamic_cast<DominionState*>(state_copy.get());
  SPIEL_CHECK_TRUE(ds_copy != nullptr);
  SPIEL_CHECK_EQ(ds->ToString(), ds_copy->ToString());
}

static int PendingChoiceVal(DominionState* s, int player) {
  return static_cast<int>(s->player_states_[player].pending_choice);
}
static size_t EffectQueueSize(DominionState* s, int player) {
  return s->player_states_[player].effect_queue.size();
}
static const open_spiel::dominion::EffectNode* FrontEffectNode(DominionState* s, int player) {
  if (s->player_states_[player].effect_queue.empty()) return nullptr;
  return s->player_states_[player].effect_queue.front().get();
}

static void TestEffectQueueJsonRoundTrip() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  GetCardSpec(open_spiel::dominion::CardName::CARD_Workshop).applyEffect(*ds, 0);
  SPIEL_CHECK_EQ(EffectQueueSize(ds, 0), 1);
  SPIEL_CHECK_EQ(PendingChoiceVal(ds, 0), static_cast<int>(open_spiel::dominion::PendingChoice::SelectUpToCardsFromBoard));
  const open_spiel::dominion::EffectNode* n = FrontEffectNode(ds, 0);
  SPIEL_CHECK_TRUE(n != nullptr);
  const auto* gs = n->gain_from_board();
  SPIEL_CHECK_TRUE(gs != nullptr);
  SPIEL_CHECK_EQ(gs->max_cost, 4);

  std::string json_str = ds->ToJson();
  nlohmann::json j = nlohmann::json::parse(json_str);
  std::unique_ptr<State> state_copy = game->NewInitialState(j);
  auto* ds_copy = dynamic_cast<DominionState*>(state_copy.get());
  SPIEL_CHECK_TRUE(ds_copy != nullptr);
  SPIEL_CHECK_EQ(EffectQueueSize(ds_copy, 0), 1);
  SPIEL_CHECK_EQ(PendingChoiceVal(ds_copy, 0), static_cast<int>(open_spiel::dominion::PendingChoice::SelectUpToCardsFromBoard));
  const open_spiel::dominion::EffectNode* n2 = FrontEffectNode(ds_copy, 0);
  SPIEL_CHECK_TRUE(n2 != nullptr);
  const auto* gs2 = n2->gain_from_board();
  SPIEL_CHECK_TRUE(gs2 != nullptr);
  SPIEL_CHECK_EQ(gs2->max_cost, 4);
  SPIEL_CHECK_TRUE(ds->LegalActions() == ds_copy->LegalActions());
}

static void TestThroneRoomChainJsonRoundTrip() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  GetCardSpec(open_spiel::dominion::CardName::CARD_ThroneRoom).applyEffect(*ds, 0);
  SPIEL_CHECK_EQ(EffectQueueSize(ds, 0), 1);
  SPIEL_CHECK_EQ(PendingChoiceVal(ds, 0), static_cast<int>(open_spiel::dominion::PendingChoice::PlayActionFromHand));
  const open_spiel::dominion::EffectNode* n = FrontEffectNode(ds, 0);
  const auto* tr = dynamic_cast<const open_spiel::dominion::ThroneRoomEffectNode*>(n);
  SPIEL_CHECK_TRUE(tr != nullptr);
  SPIEL_CHECK_TRUE(tr->throne_depth() > 0);

  std::string json_str = ds->ToJson();
  nlohmann::json j = nlohmann::json::parse(json_str);
  std::unique_ptr<State> state_copy = game->NewInitialState(j);
  auto* ds_copy = dynamic_cast<DominionState*>(state_copy.get());
  SPIEL_CHECK_TRUE(ds_copy != nullptr);
  SPIEL_CHECK_EQ(EffectQueueSize(ds_copy, 0), 1);
  SPIEL_CHECK_EQ(PendingChoiceVal(ds_copy, 0), static_cast<int>(open_spiel::dominion::PendingChoice::PlayActionFromHand));
  const open_spiel::dominion::EffectNode* n2 = FrontEffectNode(ds_copy, 0);
  const auto* tr2 = dynamic_cast<const open_spiel::dominion::ThroneRoomEffectNode*>(n2);
  SPIEL_CHECK_TRUE(tr2 != nullptr);
  SPIEL_CHECK_TRUE(tr2->throne_depth() > 0);
}

static void TestEffectQueueSerializeDeserialize() {
  std::shared_ptr<const Game> game = LoadGame("dominion");
  std::unique_ptr<State> state = game->NewInitialState();
  auto* ds = dynamic_cast<DominionState*>(state.get());
  SPIEL_CHECK_TRUE(ds != nullptr);

  GetCardSpec(open_spiel::dominion::CardName::CARD_Workshop).applyEffect(*ds, 0);
  SPIEL_CHECK_EQ(EffectQueueSize(ds, 0), 1);

  std::string serialized = ds->Serialize();
  std::unique_ptr<State> state_copy = game->DeserializeState(serialized);
  auto* ds_copy = dynamic_cast<DominionState*>(state_copy.get());
  SPIEL_CHECK_TRUE(ds_copy != nullptr);
  SPIEL_CHECK_EQ(EffectQueueSize(ds_copy, 0), 1);
  SPIEL_CHECK_EQ(PendingChoiceVal(ds_copy, 0), static_cast<int>(open_spiel::dominion::PendingChoice::SelectUpToCardsFromBoard));
}
