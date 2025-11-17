#ifndef OPEN_SPIEL_GAMES_DOMINION_H_
#define OPEN_SPIEL_GAMES_DOMINION_H_

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <map>

#include "cards.hpp"

#include "open_spiel/spiel.h"
#include "open_spiel/spiel_globals.h"

namespace open_spiel {
namespace dominion {

inline constexpr int kNumPlayers = 2;
inline constexpr int kDominionMaxDistinctActions = 256;
inline constexpr int kNumCardTypes = 26;
inline constexpr int kNumSupplyPiles = 17; // 7 base piles + 10 kingdom piles

// Outcome of the game.
enum class Outcome {
  kPlayer1,
  kPlayer2,
  kDraw,
};

// Two-phase turn: action and buy. Cleanup is implicit on EndBuy.
enum class Phase {
  actionPhase,
  buyPhase,
};

// Generic pending effect choice types.
enum class PendingChoice {
  None,
  SelectUpToCardsFromHand,
  SelectUpToCardsFromBoard,
};

// ObservationState holds references to a player's containers for observation.
// Opponent-known counts remain aggregated to preserve imperfect information.
struct ObservationState {
  std::vector<CardName>& player_hand;
  std::vector<CardName>& player_deck;
  std::vector<CardName>& player_discard;
  std::map<CardName, int> opponent_known_counts; // combined known set of opponent's hand+deck+discard

  ObservationState(std::vector<CardName>& hand,
                   std::vector<CardName>& deck,
                   std::vector<CardName>& discard)
      : player_hand(hand), player_deck(deck), player_discard(discard) {}
  ObservationState(const ObservationState& other) = default;

  // Returns known counts by distinct card for the player's deck.
  std::map<CardName, int> KnownDeckCounts() const {
    std::map<CardName, int> out;
    for (auto cn : player_deck) out[cn] += 1;
    return out;
  }
  // Returns known counts by distinct card for the player's discard.
  std::map<CardName, int> KnownDiscardCounts() const {
    std::map<CardName, int> out;
    for (auto cn : player_discard) out[cn] += 1;
    return out;
  }
};

struct PlayerState {
  std::vector<CardName> deck_;
  std::vector<CardName> hand_;
  std::vector<CardName> discard_;
  std::vector<Action> history_;
  PendingChoice pending_choice = PendingChoice::None;
  int pending_discard_count = 0; // tracks discards selected during a discard effect
  bool pending_draw_equals_discard = false; // whether finishing discard should draw equal to discards
  int pending_gain_max_cost = 0;
  int pending_target_hand_size = 0; // if > 0, force discard until reaching this hand size
  bool pending_select_only_action = false; // if true, only action cards are valid selections
  std::vector<CardName> pending_throne_replay_stack; // LIFO stack of cards to replay (play+effect)
  std::optional<CardName> pending_throne_schedule_second_for; // when set for a Throne card, schedule one more replay
  // Stable-index selection support for ascending-order constraint during discard effects.
  // Maps current hand indices to original indices at effect start; updated on selection.
  std::vector<int> pending_hand_original_indices;
  int pending_last_selected_original_index = -1;
  std::unique_ptr<EffectNode> effect_head; // head of pending effect linked list
  std::unique_ptr<ObservationState> obs_state; // per-player observation state

  PlayerState() = default;
  PlayerState(const PlayerState& other)
      : deck_(other.deck_),
        hand_(other.hand_),
        discard_(other.discard_),
        history_(other.history_),
        pending_choice(other.pending_choice),
        pending_discard_count(other.pending_discard_count),
        pending_draw_equals_discard(other.pending_draw_equals_discard),
        pending_gain_max_cost(other.pending_gain_max_cost),
        pending_target_hand_size(other.pending_target_hand_size),
        pending_hand_original_indices(other.pending_hand_original_indices),
        pending_last_selected_original_index(other.pending_last_selected_original_index),
        pending_select_only_action(other.pending_select_only_action),
        pending_throne_replay_stack(other.pending_throne_replay_stack),
        pending_throne_schedule_second_for(other.pending_throne_schedule_second_for) {
    effect_head = other.effect_head ? other.effect_head->clone() : nullptr;
    obs_state = std::make_unique<ObservationState>(hand_, deck_, discard_);
  }
  // No copy-assignment: deep copy supported via copy constructor; assignment is intentionally omitted.

  // Initialize discard selection metadata for ascending-order subset selection.
  void InitDiscardSelection(bool draw_equals_discard) {
    pending_choice = PendingChoice::SelectUpToCardsFromHand;
    pending_discard_count = 0;
    pending_draw_equals_discard = draw_equals_discard;
    pending_hand_original_indices.clear();
    pending_last_selected_original_index = -1;
    pending_hand_original_indices.reserve(static_cast<int>(hand_.size()));
    for (int i = 0; i < static_cast<int>(hand_.size()); ++i) {
      pending_hand_original_indices.push_back(i);
    }
  }

  // Clear discard selection metadata after finishing the effect.
  void ClearDiscardSelection() {
    pending_discard_count = 0;
    pending_draw_equals_discard = false;
    pending_hand_original_indices.clear();
    pending_last_selected_original_index = -1;
    pending_target_hand_size = 0;
    pending_select_only_action = false;
  }

  // Generic helpers for select-up-to effects.
  void InitHandSelection(bool draw_equals_discard) { InitDiscardSelection(draw_equals_discard); }
  void ClearHandSelection() { ClearDiscardSelection(); }

  void InitBoardSelection(int max_cost) {
    pending_choice = PendingChoice::SelectUpToCardsFromBoard;
    pending_gain_max_cost = max_cost;
  }
  void ClearBoardSelection() { pending_gain_max_cost = 0; }
};

class DominionState : public State {
 public:
  explicit DominionState(std::shared_ptr<const Game> game);

  Player CurrentPlayer() const override;
  std::vector<Action> LegalActions() const override;
  std::string ActionToString(Player player, Action action_id) const override;
  std::string ObservationString(int player) const override;
  std::string InformationStateString(int player) const override;
  std::string ToString() const override;
  bool IsTerminal() const override;
  std::vector<double> Returns() const override;
  std::unique_ptr<State> Clone() const override;

  // Draw n cards for player, shuffling discard into deck when needed.
 void DrawCardsFor(int player, int n);

 protected:
  // Applies the given action_id for the current player.
  // - Delegates effect-specific resolution first (e.g., discard selection).
  // - Handles phase transitions: EndActions -> buyPhase; EndBuy -> cleanup + next turn.
  void DoApplyAction(Action action_id) override;
  void EndBuyCleanup();

 private:
  Player current_player_ = 0;
  int coins_ = 0;
  int turn_number_ = 1;
  int actions_ = 1;
  int buys_ = 1;
  int money_0 = 0;
  Phase phase_ = Phase::actionPhase;
  int last_player_to_go_ = -1;

  std::array<int, kNumSupplyPiles> supply_piles_{}; // counts per supply pile
  std::array<CardName, kNumSupplyPiles> supply_types_{}; // type per pile
  std::vector<CardName> play_area_{};
  std::array<PlayerState, kNumPlayers> player_states_{};

  friend class Card;
  friend class EffectNode;
  friend class SelectUpToCardsNode;
  friend class SelectUpToCardsFromBoardNode;
  friend struct DominionTestHarness; // test-only accessor
  friend std::vector<Action> PendingEffectLegalActions(const DominionState& state, int player);
};

class DominionGame : public Game {
 public:
  explicit DominionGame(const GameParameters& params);

  int NumDistinctActions() const override;
  std::unique_ptr<State> NewInitialState() const override;
  int NumPlayers() const override;
  double MinUtility() const override;
  double MaxUtility() const override;
  std::vector<int> InformationStateTensorShape() const override;
  std::vector<int> ObservationTensorShape() const override;
  int MaxGameLength() const override;
  std::string GetRNGState() const override;
  void SetRNGState(const std::string& rng_state) const override;

  std::mt19937* rng() const { return &rng_; }
 private:
  mutable int rng_seed_ = 0;
  mutable std::mt19937 rng_{};
};
}  // namespace dominion
}  // namespace open_spiel

#endif