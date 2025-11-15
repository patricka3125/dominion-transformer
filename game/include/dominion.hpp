#ifndef OPEN_SPIEL_GAMES_DOMINION_H_
#define OPEN_SPIEL_GAMES_DOMINION_H_

#include <array>
#include <vector>
#include <string>

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
  ChooseDiscards,
};

struct PlayerState {
  std::vector<CardName> deck_;
  std::vector<CardName> hand_;
  std::vector<CardName> discard_;
  std::vector<Action> history_;
  PendingChoice pending_choice = PendingChoice::None;
  int pending_discard_draw_count = 0;
};

class DominionState : public State {
 public:
  explicit DominionState(std::shared_ptr<const Game> game);

  Player CurrentPlayer() const override;
  std::vector<Action> LegalActions() const override;
  std::string ActionToString(Player player, Action action_id) const override;
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

 private:
  Player current_player_ = 0;
  int coins_ = 0;
  int turn_number_ = 0;
  int actions_ = 1;
  int buys_1 = 1;
  int money_0 = 0;
  Phase phase_ = Phase::actionPhase;

  std::array<int, kNumSupplyPiles> supply_piles_{}; // counts per supply pile
  std::array<CardName, kNumSupplyPiles> supply_types_{}; // type per pile
  std::vector<CardName> play_area_{};
  std::array<PlayerState, kNumPlayers> player_states_{};

  friend class Card;
  friend struct DominionTestHarness; // test-only accessor
  friend bool HandlePendingEffectAction(DominionState& state, int player, Action action_id);
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
};
}  // namespace dominion
}  // namespace open_spiel

#endif