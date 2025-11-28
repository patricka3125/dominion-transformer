#ifndef OPEN_SPIEL_GAMES_DOMINION_EFFECTS_H_
#define OPEN_SPIEL_GAMES_DOMINION_EFFECTS_H_

#include <memory>
#include <functional>

#include "open_spiel/spiel.h"

namespace open_spiel {
namespace dominion {

class DominionState;
enum class PendingChoice : int;

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
  virtual struct HandSelectionStruct* hand_selection() { return nullptr; }
  virtual const struct HandSelectionStruct* hand_selection() const { return nullptr; }
  virtual struct GainFromBoardStruct* gain_from_board() { return nullptr; }
  virtual const struct GainFromBoardStruct* gain_from_board() const { return nullptr; }
};

// Effect-local state for hand selection flows.
// - target_hand_size: threshold to auto-finish (e.g., Militia to 3)
// - last_selected_original_index: enforces ascending original index selection
// - selection_count: tracks number of selections in current effect
struct HandSelectionStruct {
  int target_hand_size = 0;
  int last_selected_original_index = -1;
  int selection_count = 0;
  void set_target_hand_size(int v) { target_hand_size = v; }
  int target_hand_size_value() const { return target_hand_size; }
  int last_selected_original_index_value() const { return last_selected_original_index; }
  void set_last_selected_original_index(int j) { last_selected_original_index = j; }
  int selection_count_value() const { return selection_count; }
  void increment_selection_count() { ++selection_count; }
  void set_selection_count(int v) { selection_count = v; }
  void reset_selection() { last_selected_original_index = -1; selection_count = 0; }
};

// Effect-local state for gain-from-board flows.
// - max_cost: upper bound for legal gains from supply
struct GainFromBoardStruct {
  int max_cost = 0;
  explicit GainFromBoardStruct(int c = 0) : max_cost(c) {}
};

// Cellar: discard any number, then draw equal.
class CellarEffectNode : public EffectNode {
public:
  std::unique_ptr<EffectNode> clone() const override {
    auto n = std::unique_ptr<CellarEffectNode>(new CellarEffectNode());
    n->hand_ = hand_;
    n->on_action = on_action;
    return std::unique_ptr<EffectNode>(std::move(n));
  }
  HandSelectionStruct* hand_selection() override { return &hand_; }
  const HandSelectionStruct* hand_selection() const override { return &hand_; }
private:
  HandSelectionStruct hand_;
};

// Chapel: trash up to 4 cards.
class ChapelEffectNode : public EffectNode {
public:
  std::unique_ptr<EffectNode> clone() const override {
    auto n = std::unique_ptr<ChapelEffectNode>(new ChapelEffectNode());
    n->hand_ = hand_;
    n->on_action = on_action;
    return std::unique_ptr<EffectNode>(std::move(n));
  }
  HandSelectionStruct* hand_selection() override { return &hand_; }
  const HandSelectionStruct* hand_selection() const override { return &hand_; }
private:
  HandSelectionStruct hand_;
};

// Remodel stage-1: trash a card from hand.
class RemodelTrashEffectNode : public EffectNode {
public:
  std::unique_ptr<EffectNode> clone() const override {
    auto n = std::unique_ptr<RemodelTrashEffectNode>(new RemodelTrashEffectNode());
    n->hand_ = hand_;
    n->on_action = on_action;
    return std::unique_ptr<EffectNode>(std::move(n));
  }
  HandSelectionStruct* hand_selection() override { return &hand_; }
  const HandSelectionStruct* hand_selection() const override { return &hand_; }
private:
  HandSelectionStruct hand_;
};

// Militia: opponent discards down to target hand size.
class MilitiaEffectNode : public EffectNode {
public:
  std::unique_ptr<EffectNode> clone() const override {
    auto n = std::unique_ptr<MilitiaEffectNode>(new MilitiaEffectNode());
    n->hand_ = hand_;
    n->on_action = on_action;
    return std::unique_ptr<EffectNode>(std::move(n));
  }
  HandSelectionStruct* hand_selection() override { return &hand_; }
  const HandSelectionStruct* hand_selection() const override { return &hand_; }
private:
  HandSelectionStruct hand_;
};

// Throne Room: select an action and play it twice; if selecting Throne Room,
// chains additional selection depth until a non-Throne action is chosen.
class ThroneRoomEffectNode : public EffectNode {
public:
  std::unique_ptr<EffectNode> clone() const override {
    auto n = std::unique_ptr<ThroneRoomEffectNode>(new ThroneRoomEffectNode());
    n->hand_ = hand_;
    for (int i = 0; i < throne_select_depth_; ++i) n->increment_throne_depth();
    n->on_action = on_action;
    return std::unique_ptr<EffectNode>(std::move(n));
  }
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
class WorkshopEffectNode : public EffectNode {
public:
  explicit WorkshopEffectNode(int max_cost) : gain_(max_cost) {}
  std::unique_ptr<EffectNode> clone() const override {
    auto n = std::unique_ptr<WorkshopEffectNode>(new WorkshopEffectNode(gain_.max_cost));
    n->on_action = on_action;
    return std::unique_ptr<EffectNode>(std::move(n));
  }
  GainFromBoardStruct* gain_from_board() override { return &gain_; }
  const GainFromBoardStruct* gain_from_board() const override { return &gain_; }
private:
  GainFromBoardStruct gain_;
};

// Remodel stage-2: gain a card from the supply up to trashed cost + 2.
class RemodelGainEffectNode : public EffectNode {
public:
  explicit RemodelGainEffectNode(int max_cost) : gain_(max_cost) {}
  std::unique_ptr<EffectNode> clone() const override {
    auto n = std::unique_ptr<RemodelGainEffectNode>(new RemodelGainEffectNode(gain_.max_cost));
    n->on_action = on_action;
    return std::unique_ptr<EffectNode>(std::move(n));
  }
  GainFromBoardStruct* gain_from_board() override { return &gain_; }
  const GainFromBoardStruct* gain_from_board() const override { return &gain_; }
private:
  GainFromBoardStruct gain_;
};

} // namespace dominion
} // namespace open_spiel

#endif
