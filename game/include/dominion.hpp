#ifndef OPEN_SPIEL_GAMES_DOMINION_H_
#define OPEN_SPIEL_GAMES_DOMINION_H_

#include <array>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <deque>

#include "cards.hpp"

#include "open_spiel/json/include/nlohmann/json.hpp"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_globals.h"

namespace open_spiel {
namespace dominion {

inline constexpr int kNumPlayers = 2;
inline constexpr int kDominionMaxDistinctActions = 4096; // buffer for future action additions
inline constexpr int kNumCardTypes = 33; // total card enumerators
inline constexpr int kNumSupplyPiles = kNumCardTypes; // supply indexed by CardName

// Index conversion helpers
inline int ToIndex(CardName card) { return static_cast<int>(card); }
inline CardName ToCardName(int idx) { return static_cast<CardName>(idx); }
inline bool IsValidPileIndex(int idx) { return idx >= 0 && idx < kNumSupplyPiles; }

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

// Pending effect choice types, split by hand-selection semantics.
enum class PendingChoice : int {
  None = 0,
  DiscardUpToCardsFromHand = 1,
  TrashUpToCardsFromHand = 2,
  PlayActionFromHand = 3,
  SelectUpToCardsFromBoard = 4,
};

// ObservationState holds references to a player's containers for observation.
// Opponent-known counts remain aggregated to preserve imperfect information.
struct ObservationState {
  std::array<int, kNumSupplyPiles> &player_hand_counts;
  std::vector<CardName> &player_deck;
  std::array<int, kNumSupplyPiles> &player_discard_counts;
  std::map<CardName, int> opponent_known_counts; // combined known set of opponent's hand+deck+discard

  ObservationState(std::array<int, kNumSupplyPiles> &hand_counts,
                   std::vector<CardName> &deck,
                   std::array<int, kNumSupplyPiles> &discard_counts)
      : player_hand_counts(hand_counts), player_deck(deck), player_discard_counts(discard_counts) {}
  ObservationState(const ObservationState &other) = default;

  std::map<CardName, int> KnownDeckCounts() const {
    std::map<CardName, int> out;
    for (auto cn : player_deck) out[cn] += 1;
    return out;
  }
  std::map<CardName, int> KnownDiscardCounts() const {
    std::map<CardName, int> out;
    for (int j = 0; j < kNumSupplyPiles; ++j) {
      if (player_discard_counts[j] > 0) out[static_cast<CardName>(j)] = player_discard_counts[j];
    }
    return out;
  }

};

// JSON-serializable contents used by StateStructs.
struct DominionPlayerStructContents {
  std::vector<int> deck;
  std::array<int, kNumSupplyPiles> hand_counts;
  std::array<int, kNumSupplyPiles> discard_counts;
  int pending_choice;
  std::vector<EffectNodeStructContents> effect_queue;
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      DominionPlayerStructContents, deck, hand_counts, discard_counts,
      pending_choice, effect_queue)
};

struct DominionStateStructContents {
  int current_player;
  int coins;
  int turn_number;
  int actions;
  int buys;
  int merchants_played;
  int phase;
  int last_player_to_go;
  bool shuffle_pending;
  bool shuffle_pending_end_of_turn;
  int original_player_for_shuffle;
  int pending_draw_count_after_shuffle;
  std::array<int, kNumSupplyPiles> supply_piles;
  std::array<int, kNumSupplyPiles> initial_supply_piles;
  std::vector<int> play_area;
  std::vector<DominionPlayerStructContents> player_states;
  int move_number;
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(
      DominionStateStructContents, current_player, coins, turn_number, actions,
      buys, merchants_played, phase, last_player_to_go, shuffle_pending,
      shuffle_pending_end_of_turn, original_player_for_shuffle,
      pending_draw_count_after_shuffle, supply_piles, initial_supply_piles,
      play_area, player_states, move_number)
};

struct DominionPlayerStateStruct : public StateStruct,
                                   public DominionPlayerStructContents {
  DominionPlayerStateStruct() = default;
  explicit DominionPlayerStateStruct(const std::string &json_str) {
    nlohmann::json::parse(json_str).get_to(*this);
  }
  nlohmann::json to_json_base() const override { return *this; }
};

struct DominionStateStruct : public StateStruct,
                              public DominionStateStructContents {
  DominionStateStruct() = default;
  explicit DominionStateStruct(const std::string &json_str) {
    nlohmann::json::parse(json_str).get_to(*this);
  }
  nlohmann::json to_json_base() const override { return *this; }
};

struct PlayerState {
  std::vector<CardName> deck_;
  std::array<int, kNumSupplyPiles> hand_counts_{};
  std::array<int, kNumSupplyPiles> discard_counts_{};
  std::vector<Action> history_;
  PendingChoice pending_choice = PendingChoice::None;
  // Effect-specific state moved into nodes; PlayerState retains only choice
  // type and the effect queue.
  std::deque<std::unique_ptr<EffectNode>> effect_queue; // FIFO of pending effects
  std::unique_ptr<ObservationState> obs_state; // per-player observation state

  PlayerState() = default;
  PlayerState(const PlayerState &other)
      : deck_(other.deck_),
        hand_counts_(other.hand_counts_),
        discard_counts_(other.discard_counts_),
        history_(other.history_),
        pending_choice(other.pending_choice) {
    effect_queue.clear();
    for (const auto &node_ptr : other.effect_queue) {
      if (node_ptr) {
        auto cloned = node_ptr->clone();
        cloned->on_action = node_ptr->on_action;
        effect_queue.push_back(std::move(cloned));
      } else {
        effect_queue.push_back(nullptr);
      }
    }
    obs_state = std::make_unique<ObservationState>(hand_counts_, deck_, discard_counts_);
  }
  explicit PlayerState(const nlohmann::json &json) {
    DominionPlayerStateStruct ss(json.dump());
    LoadFromStruct(ss);
  }

  void LoadFromStruct(const DominionPlayerStateStruct &ss) {
    deck_.clear();
    hand_counts_.fill(0);
    discard_counts_.fill(0);
    history_.clear();
    pending_choice = PendingChoice::None;
    effect_queue.clear();
    deck_.reserve(ss.deck.size());
    for (int v : ss.deck) deck_.push_back(static_cast<CardName>(v));
    hand_counts_ = ss.hand_counts;
    discard_counts_ = ss.discard_counts;
    pending_choice = static_cast<PendingChoice>(ss.pending_choice);
    for (const auto &ens : ss.effect_queue) {
      auto node = EffectNodeFromStruct(ens, pending_choice);
      if (node) effect_queue.push_back(std::move(node));
    }
    obs_state = std::make_unique<ObservationState>(hand_counts_, deck_, discard_counts_);
  }

  // JSON struct factory.
  std::unique_ptr<StateStruct> ToStruct() const {
    DominionPlayerStructContents contents;
    contents.deck.clear();
    contents.deck.reserve(deck_.size());
    for (auto cn : deck_) contents.deck.push_back(static_cast<int>(cn));
    contents.hand_counts = hand_counts_;
    contents.discard_counts = discard_counts_;
    // history_ not included in JSON struct.
    contents.pending_choice = static_cast<int>(pending_choice);
    contents.effect_queue.clear();
    for (const auto &node_ptr : effect_queue) {
      if (node_ptr) contents.effect_queue.push_back(EffectNodeToStruct(*node_ptr));
    }
    auto ss = std::make_unique<DominionPlayerStateStruct>();
    static_cast<DominionPlayerStructContents&>(*ss) = contents;
    return ss;
  }
  // No copy-assignment: deep copy supported via copy constructor; assignment is
  // intentionally omitted.


  // Clear discard selection metadata after finishing the effect.
  void ClearDiscardSelection() {
    // Node-owned state handles metadata; PlayerState only tracks choice.
  }

  // Hand count management helpers
  int HandCount(CardName card) const {
    return hand_counts_[static_cast<int>(card)];
  }

  int TotalHandSize() const {
    int total = 0;
    for (int count : hand_counts_) {
      total += count;
    }
    return total;
  }

  void AddToHand(CardName card, int count = 1) {
    hand_counts_[static_cast<int>(card)] += count;
  }

  bool RemoveFromHand(CardName card, int count = 1) {
    int& hand_count = hand_counts_[static_cast<int>(card)];
    if (hand_count >= count) {
      hand_count -= count;
      return true;
    }
    return false;
  }

  void MoveHandToDiscard() {
    for (int j = 0; j < kNumSupplyPiles; ++j) {
      discard_counts_[j] += hand_counts_[j];
      hand_counts_[j] = 0;
    }
  }

  int TotalDiscardSize() const {
    int total = 0;
    for (int count : discard_counts_) {
      total += count;
    }
    return total;
  }

  void AddToDiscard(CardName card, int count = 1) {
    discard_counts_[static_cast<int>(card)] += count;
  }

  bool RemoveFromDiscard(CardName card, int count = 1) {
    int& discard_count = discard_counts_[static_cast<int>(card)];
    if (discard_count >= count) {
      discard_count -= count;
      return true;
    }
    return false;
  }

  EffectNode* FrontEffect() {
    return effect_queue.empty() ? nullptr : effect_queue.front().get();
  }

  const EffectNode* FrontEffect() const {
    return effect_queue.empty() ? nullptr : effect_queue.front().get();
  }

};

class DominionState : public State {
public:
  explicit DominionState(std::shared_ptr<const Game> game);
  DominionState(std::shared_ptr<const Game> game, const nlohmann::json &json);

  Player CurrentPlayer() const override;
  std::vector<Action> LegalActions() const override;
  std::string ActionToString(Player player, Action action_id) const override;
  std::string ObservationString(int player) const override;
  std::string InformationStateString(int player) const override;
  std::string ToString() const override;
  bool IsTerminal() const override;
  std::vector<double> Returns() const override;
  std::unique_ptr<State> Clone() const override;
  ActionsAndProbs ChanceOutcomes() const override;
  std::unique_ptr<StateStruct> ToStruct() const override;
  std::string Serialize() const override;

  // Draw n cards for player, shuffling discard into deck when needed.
  void DrawCardsFor(int player, int n);

  Player current_player_ = 0;
  int coins_ = 0;
  int turn_number_ = 1;
  int actions_ = 1;
  int buys_ = 1;
  Phase phase_ = Phase::actionPhase;
  int last_player_to_go_ = -1;
  std::array<int, kNumSupplyPiles> supply_piles_{}; // counts per supply pile (indexed by CardName)
  std::array<int, kNumSupplyPiles> initial_supply_piles_{}; // initial counts for terminal checks, represents the kingdom.
  std::vector<CardName> play_area_{};
  std::array<PlayerState, kNumPlayers> player_states_{};
  int merchants_played_ = 0;

protected:
  // Applies the given action_id for the current player.
  // - Delegates effect-specific resolution first (e.g., discard selection).
  // - Handles phase transitions: EndActions -> buyPhase; EndBuy -> cleanup +
  // next turn.
  void DoApplyAction(Action action_id) override;
  void EndBuyCleanup();

  // Automatically transition to buy phase when there are no playable action
  // cards in hand or when the player has 0 actions remaining. This should only
  // run when there is no pending effect/choice, to avoid skipping required
  // selections.
  void MaybeAutoAdvanceToBuyPhase();
  // Optimization: when not at chance node, if LegalActions returns a single
  // action, auto-apply it and continue until branching occurs.
  void MaybeAutoApplySingleAction();
  private:
  void ApplyMerchantBonusOnSilverPlay();
  // Sampled stochastic shuffle state (internal-only).
  bool shuffle_pending_ = false;
  bool shuffle_pending_end_of_turn_ = false;
  int original_player_for_shuffle_ = -1;
  int pending_draw_count_after_shuffle_ = 0;

  friend struct DominionTestHarness; // test-only accessor
  friend std::vector<Action>
  PendingEffectLegalActions(const DominionState &state, int player);
};

class DominionGame : public Game {
public:
  explicit DominionGame(const GameParameters &params);

  int NumDistinctActions() const override;
  std::unique_ptr<State> NewInitialState() const override;
  int NumPlayers() const override;
  double MinUtility() const override;
  double MaxUtility() const override;
  std::vector<int> InformationStateTensorShape() const override;
  std::vector<int> ObservationTensorShape() const override;
  int MaxGameLength() const override;
  int MaxChanceOutcomes() const override;
  std::unique_ptr<State> NewInitialState(const nlohmann::json &json) const override;
  std::unique_ptr<State> DeserializeState(const std::string &str) const override;


private:
};
} // namespace dominion
} // namespace open_spiel

#endif
