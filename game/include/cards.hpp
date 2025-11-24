#ifndef OPEN_SPIEL_GAMES_DOMINION_CARDS_H_
#define OPEN_SPIEL_GAMES_DOMINION_CARDS_H_

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

#include "open_spiel/spiel.h"

namespace open_spiel {
namespace dominion {

class DominionState;

// Effect processing nodes: lightweight units representing pending effects.
// Nodes are managed by the player's effect queue (FIFO).
class EffectNode {
public:
  virtual ~EffectNode() = default;
  // Called when the effect is appended to a player; responsible for
  // initializing any player-level pending choice state.
  virtual void onEnter(DominionState& state, int player) {}
  // Polymorphic deep copy of the effect chain.
  virtual std::unique_ptr<EffectNode> clone() const = 0;
  // Optional callback that handles actions while this effect is pending.
  // Returns true if the action was consumed.
  std::function<bool(DominionState&, int, Action)> on_action;
};

// Select up to any number of cards from hand. Optionally draw an equal
// number after finishing the selection.
class SelectUpToCardsNode : public EffectNode {
public:
  explicit SelectUpToCardsNode(bool draw_equals_discard)
      : draw_equals_discard_(draw_equals_discard) {}
  void onEnter(DominionState& state, int player) override;
  std::unique_ptr<EffectNode> clone() const override {
    auto n = std::unique_ptr<SelectUpToCardsNode>(new SelectUpToCardsNode(draw_equals_discard_));
    n->target_hand_size_ = target_hand_size_;
    n->last_selected_original_index_ = last_selected_original_index_;
    n->discard_count_ = discard_count_;
    n->throne_select_depth_ = throne_select_depth_;
    n->on_action = on_action;
    return std::unique_ptr<EffectNode>(std::move(n));
  }
  // Effect-local state accessors
  void set_target_hand_size(int v) { target_hand_size_ = v; }
  int target_hand_size() const { return target_hand_size_; }
  int last_selected_original_index() const { return last_selected_original_index_; }
  void set_last_selected_original_index(int j) { last_selected_original_index_ = j; }
  int discard_count() const { return discard_count_; }
  void increment_discard_count() { ++discard_count_; }
  bool draw_equals_discard() const { return draw_equals_discard_; }
  int throne_depth() const { return throne_select_depth_; }
  void increment_throne_depth() { ++throne_select_depth_; }
  void decrement_throne_depth() { if (throne_select_depth_ > 0) --throne_select_depth_; }
private:
  bool draw_equals_discard_ = false;
  int target_hand_size_ = 0;
  int last_selected_original_index_ = -1;
  int discard_count_ = 0;
  int throne_select_depth_ = 0;
};

// Select from board (supply piles) up to a cost cap; puts the selected
// card into the player's discard and decrements the supply. No extra
// user input beyond the selection itself.
class SelectUpToCardsFromBoardNode : public EffectNode {
public:
  explicit SelectUpToCardsFromBoardNode(int max_cost)
      : max_cost_(max_cost) {}
  void onEnter(DominionState& state, int player) override;
  std::unique_ptr<EffectNode> clone() const override {
    auto n = std::unique_ptr<SelectUpToCardsFromBoardNode>(new SelectUpToCardsFromBoardNode(max_cost_));
    n->on_action = on_action;
    return std::unique_ptr<EffectNode>(std::move(n));
  }
  int max_cost() const { return max_cost_; }
private:
  int max_cost_ = 0;
};

enum class CardName {
  // Basic supply cards
  CARD_Copper,
  CARD_Silver,
  CARD_Gold,
  CARD_Estate,
  CARD_Duchy,
  CARD_Province,
  CARD_Curse,

  // 26 Base set cards
  CARD_Artisan,
  CARD_Bandit,
  CARD_Bureaucrat,
  CARD_Cellar,
  CARD_Chapel,
  CARD_CouncilRoom,
  CARD_Festival,
  CARD_Gardens,
  CARD_Harbinger,
  CARD_Laboratory,
  CARD_Library,
  CARD_Market,
  CARD_Merchant,
  CARD_Militia,
  CARD_Mine,
  CARD_Moat,
  CARD_Moneylender,
  CARD_Poacher,
  CARD_Remodel,
  CARD_Sentry,
  CARD_Smithy,
  CARD_ThroneRoom,
  CARD_Vassal,
  CARD_Village,
  CARD_Witch,
  CARD_Workshop
};

// Compact storage for card types (optional)
enum class CardType : uint8_t {
    BASIC_TREASURE,
    ACTION ,
    VICTORY,
    CURSE,
    ATTACK,
    SPECIAL_TREASURE // placeholder (does not exist in base set)
};

struct CardOptions {
    std::string name;
    std::vector<CardType> types;
    std::optional<int> cost;
    std::optional<int> value;
    std::optional<int> vp;
    std::optional<int> grant_action;
    std::optional<int> grant_draw;
    std::optional<int> grant_buy;
};

class Card {
public:
    std::string name_;
    CardName kind_;
    std::vector<CardType> types_;
    int cost_ = 0;
    int value_ = 0;
    int vp_ = 0;

    int grant_action_ = 0; // +Actions
    int grant_draw_ = 0;   // +Cards
    int grant_buy_ = 0;    // +Buys

    Card(CardName kind_, std::string name_, std::vector<CardType> types_, int cost_=0, int value_=0, int vp_=0,
         int grant_action_ = 0, int grant_draw_ = 0, int grant_buy_ = 0)
      : name_(std::move(name_)),
        kind_(kind_),
        types_(std::move(types_)),
        cost_(cost_), value_(value_), vp_(vp_),
        grant_action_(grant_action_), grant_draw_(grant_draw_), grant_buy_(grant_buy_) {}

    Card(const CardOptions& opt)
      : name_(opt.name),
        types_(opt.types),
        cost_(opt.cost.value_or(0)),
        value_(opt.value.value_or(0)),
        vp_(opt.vp.value_or(0)),
        grant_action_(opt.grant_action.value_or(0)),
        grant_draw_(opt.grant_draw.value_or(0)),
        grant_buy_(opt.grant_buy.value_or(0)) {}

    static Card fromOptions(const CardOptions& opt) { return Card(opt); }

    // Applies standard grants: +actions, +buys, +coins, +cards.
    void play(DominionState& state, int player) const;
    // Triggers any effect chains; used for interactive effects like Cellar.
    void applyEffect(DominionState& state, int player) const;

    // Helper handlers for effect chains.
    static bool GainFromBoardHandler(DominionState& state, int player, Action action_id);
    static bool RemodelTrashFromHand(DominionState& state, int player, Action action_id);
    static bool CellarHandSelectHandler(DominionState& state, int player, Action action_id);
    static bool ChapelHandTrashHandler(DominionState& state, int player, Action action_id);
    static bool MilitiaOpponentDiscardHandler(DominionState& state, int player, Action action_id);
    static void WitchAttackGiveCurse(DominionState& state, int player);
    static bool ThroneRoomSelectActionHandler(DominionState& state, int player, Action action_id);

    // Generic helper to collapse repeated hand-selection logic (ascending original index).
    // - allow_finish: whether HandSelectFinish is a legal action to end early
    // - max_select_count: if >= 0, auto-finish after this many selections
    // - finish_on_target_hand_size: if true, finish when hand size <= pending_target_hand_size
    // - on_select: callback invoked with current index to perform per-card effects (e.g., discard/trash)
    // - on_finish: callback invoked upon finishing (e.g., Cellar draws equal to discards)
    static bool GenericHandSelectionHandler(
        DominionState& state, int player, Action action_id,
        bool allow_finish,
        int max_select_count,
        bool finish_on_target_hand_size,
        const std::function<void(DominionState&, int, int)>& on_select,
        const std::function<void(DominionState&, int)>& on_finish);
};

const Card& GetCardSpec(CardName name);

std::vector<Action> PendingEffectLegalActions(const DominionState& state, int player);

} // namespace dominion
} // namespace open_spiel

#endif
