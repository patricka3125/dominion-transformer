#ifndef OPEN_SPIEL_GAMES_DOMINION_EFFECTS_H_
#define OPEN_SPIEL_GAMES_DOMINION_EFFECTS_H_

#include <memory>
#include <functional>

#include "open_spiel/spiel.h"
#include "open_spiel/json/include/nlohmann/json.hpp"

namespace open_spiel {
namespace dominion {

class DominionState;
enum class PendingChoice : int;
enum class CardName;

// Base node for pending effects. Each node can optionally expose
// a HandSelectionStruct or GainFromBoardStruct view for shared handlers.
// Nodes install an on_action handler while they sit at the front of the
// player's effect queue.
class EffectNode {
public:
  virtual ~EffectNode() = default;
  // Polymorphic deep copy, used in PlayerState copy construction.
  virtual std::unique_ptr<EffectNode> clone() const = 0;
  std::function<bool(DominionState&, int, Action)> on_action;
  bool enforce_ascending = false;
  virtual struct HandSelectionStruct* hand_selection() { return nullptr; }
  virtual const struct HandSelectionStruct* hand_selection() const { return nullptr; }
  virtual struct GainFromBoardStruct* gain_from_board() { return nullptr; }
  virtual const struct GainFromBoardStruct* gain_from_board() const { return nullptr; }
};

// CRTP base for automatic clone implementation
template<typename Derived>
class CloneableEffectNode : public EffectNode {
public:
  std::unique_ptr<EffectNode> clone() const override {
    auto cloned = std::make_unique<Derived>(static_cast<const Derived&>(*this));
    cloned->on_action = on_action;
    cloned->enforce_ascending = enforce_ascending;
    return cloned;
  }
};

// Effect-local state for hand selection flows.
// - target_hand_size: threshold to auto-finish (e.g., Militia to 3)
// - last_selected_original_index: enforces ascending original index selection
// - selection_count: tracks number of selections in current effect
// - allow_finish_selection: whether the card selection allows early finish.
struct HandSelectionStruct {
  int target_hand_size = 0; // TODO: Improve target_hand_size_value semantic.
  int last_selected_original_index = -1;
  int selection_count = 0;

  bool allow_finish_selection = false;
  bool only_treasure = false;

  void set_target_hand_size(int v) { target_hand_size = v; }
  int target_hand_size_value() const { return target_hand_size; }
  int last_selected_original_index_value() const { return last_selected_original_index; }
  void set_last_selected_original_index(int j) { last_selected_original_index = j; }
  int selection_count_value() const { return selection_count; }
  void increment_selection_count() { ++selection_count; }
  void set_selection_count(int v) { selection_count = v; }
  void reset_selection() { last_selected_original_index = -1; selection_count = 0; }
  bool get_allow_finish_selection() const { return allow_finish_selection; }
  void set_allow_finish_selection() {allow_finish_selection = true;}
  bool get_only_treasure() const { return only_treasure; }
  void set_only_treasure() { only_treasure = true; }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(HandSelectionStruct,
                                 target_hand_size,
                                 last_selected_original_index,
                                 selection_count,
                                 allow_finish_selection,
                                 only_treasure)
};

// Effect-local state for gain-from-board flows.
// - max_cost: upper bound for legal gains from supply
struct GainFromBoardStruct {
  int max_cost = 0;
  bool only_treasure = false;
  explicit GainFromBoardStruct(int c = 0) : max_cost(c) {}
  bool get_only_treasure() const { return only_treasure; }
  void set_only_treasure() { only_treasure = true; }
};

// Cellar: discard any number, then draw equal.
class CellarEffectNode : public CloneableEffectNode<CellarEffectNode> {
public:
  CellarEffectNode() = default;
  CellarEffectNode(PendingChoice choice, const HandSelectionStruct* hs = nullptr);
  HandSelectionStruct* hand_selection() override { return &hand_; }
  const HandSelectionStruct* hand_selection() const override { return &hand_; }
private:
  HandSelectionStruct hand_;
};

// Chapel: trash up to 4 cards.
class ChapelEffectNode : public CloneableEffectNode<ChapelEffectNode> {
public:
  ChapelEffectNode() = default;
  ChapelEffectNode(PendingChoice choice, const HandSelectionStruct* hs = nullptr);
  HandSelectionStruct* hand_selection() override { return &hand_; }
  const HandSelectionStruct* hand_selection() const override { return &hand_; }
private:
  HandSelectionStruct hand_;
};

// Remodel stage-1: trash a card from hand.
class RemodelTrashEffectNode : public CloneableEffectNode<RemodelTrashEffectNode> {
public:
  RemodelTrashEffectNode() = default;
  RemodelTrashEffectNode(PendingChoice choice, const HandSelectionStruct* hs = nullptr);
  HandSelectionStruct* hand_selection() override { return &hand_; }
  const HandSelectionStruct* hand_selection() const override { return &hand_; }
private:
  HandSelectionStruct hand_;
};

// Militia: opponent discards down to target hand size.
class MilitiaEffectNode : public CloneableEffectNode<MilitiaEffectNode> {
public:
  MilitiaEffectNode() = default;
  MilitiaEffectNode(PendingChoice choice, const HandSelectionStruct* hs = nullptr);
  HandSelectionStruct* hand_selection() override { return &hand_; }
  const HandSelectionStruct* hand_selection() const override { return &hand_; }
private:
  HandSelectionStruct hand_;
};

// Throne Room: select an action and play it twice; if selecting Throne Room,
// chains additional selection depth until a non-Throne action is chosen.
class ThroneRoomEffectNode : public CloneableEffectNode<ThroneRoomEffectNode> {
public:
  ThroneRoomEffectNode() = default;
  explicit ThroneRoomEffectNode(int depth);
  int throne_depth() const { return throne_select_depth_; }
  void increment_throne_depth() { ++throne_select_depth_; }
  void decrement_throne_depth() { if (throne_select_depth_ > 0) --throne_select_depth_; }
  // Begin a fresh selection for an action card from hand.
  void BeginSelection(DominionState& state, int player);
  // Increment chain depth and begin another selection.
  void StartChain(DominionState& state, int player);
  // Decrement depth; finish if zero, else restart selection.
  void ContinueOrFinish(DominionState& state, int player);
  // Clear pending choice and finish the effect.
  void FinishSelection(DominionState& state, int player);
  HandSelectionStruct* hand_selection() override { return &hand_; }
  const HandSelectionStruct* hand_selection() const override { return &hand_; }
private:
  HandSelectionStruct hand_;
  int throne_select_depth_ = 0;
};

// Workshop: gain a card from the supply up to cost 4.
class WorkshopEffectNode : public CloneableEffectNode<WorkshopEffectNode> {
public:
  WorkshopEffectNode() = default;
  explicit WorkshopEffectNode(int max_cost) : gain_(max_cost) {}
  GainFromBoardStruct* gain_from_board() override { return &gain_; }
  const GainFromBoardStruct* gain_from_board() const override { return &gain_; }
private:
  GainFromBoardStruct gain_;
};

// Remodel stage-2: gain a card from the supply up to trashed cost + 2.
class RemodelGainEffectNode : public CloneableEffectNode<RemodelGainEffectNode> {
public:
  RemodelGainEffectNode() = default;
  explicit RemodelGainEffectNode(int max_cost) : gain_(max_cost) {}
  GainFromBoardStruct* gain_from_board() override { return &gain_; }
  const GainFromBoardStruct* gain_from_board() const override { return &gain_; }
private:
  GainFromBoardStruct gain_;
};

struct EffectNodeStructContents {
  int kind = 0;
  HandSelectionStruct hand;
  int gain_max_cost = 0;
  bool gain_only_treasure = false;
  int throne_select_depth = 0;
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(EffectNodeStructContents,
                                 kind,
                                 hand,
                                 gain_max_cost,
                                 gain_only_treasure,
                                 throne_select_depth)
};

EffectNodeStructContents EffectNodeToStruct(const EffectNode& node);
std::unique_ptr<EffectNode> EffectNodeFromStruct(const EffectNodeStructContents& s,
                                                 PendingChoice pending_choice);

// Factory pattern for centralized effect node creation
class EffectNodeFactory {
public:
  // Create effect nodes for hand selection-based effects
  static std::unique_ptr<EffectNode> CreateHandSelectionEffect(
      CardName card,
      PendingChoice choice,
      const HandSelectionStruct* hs = nullptr);

  // Create effect nodes for gain-from-board effects
  static std::unique_ptr<EffectNode> CreateGainEffect(
      CardName card,
      int max_cost);

  // Create throne room effect node with specific depth
  static std::unique_ptr<EffectNode> CreateThroneRoomEffect(int depth = 0);

  // Generic factory method that delegates to specific creators
  static std::unique_ptr<EffectNode> Create(
      CardName card,
      PendingChoice choice,
      const HandSelectionStruct* hs = nullptr,
      int extra_param = 0);
};

} // namespace dominion
} // namespace open_spiel

#endif
