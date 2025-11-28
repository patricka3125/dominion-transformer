#ifndef OPEN_SPIEL_GAMES_DOMINION_CARDS_H_
#define OPEN_SPIEL_GAMES_DOMINION_CARDS_H_

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

#include "open_spiel/spiel.h"
#include "effects.hpp"

namespace open_spiel {
namespace dominion {

// Forward declaration to avoid circular include: defined in dominion.hpp
enum class PendingChoice : int;

class DominionState;


// Selection initialization helpers are provided as Card static methods.

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
    virtual ~Card() = default;
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
    void applyGrants(DominionState& state, int player) const;
    // Default effect hook: no-op. Derived cards override this for custom effects.
    virtual void applyEffect(DominionState& state, int player) const;
    // Unified play: apply standard grants, then card-specific effects.
    void Play(DominionState& state, int player) const;

    // Helper handlers for effect chains.
    static void InitHandSelection(DominionState& state, int player, EffectNode* node, PendingChoice choice);
    static void InitBoardSelection(DominionState& state, int player);

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
        int select_base,
        Action select_finish,
        const std::function<void(DominionState&, int, int)>& on_select,
        const std::function<void(DominionState&, int)>& on_finish);
    static bool GainFromBoardHandler(DominionState& state, int player, Action action_id);
};

// Derived cards with custom effects
class CellarCard : public Card {
public:
  using Card::Card;
  void applyEffect(DominionState& state, int player) const override;
  static bool CellarHandSelectHandler(DominionState& state, int player, Action action_id);
};

class WorkshopCard : public Card {
public:
  using Card::Card;
  void applyEffect(DominionState& state, int player) const override;
};

class RemodelCard : public Card {
public:
  using Card::Card;
  void applyEffect(DominionState& state, int player) const override;
  static bool RemodelTrashFromHand(DominionState& state, int player, Action action_id);
};

class ChapelCard : public Card {
public:
  using Card::Card;
  void applyEffect(DominionState& state, int player) const override;
  static bool ChapelHandTrashHandler(DominionState& state, int player, Action action_id);
};

class MilitiaCard : public Card {
public:
  using Card::Card;
  void applyEffect(DominionState& state, int player) const override;
  static bool MilitiaOpponentDiscardHandler(DominionState& state, int player, Action action_id);
};

class WitchCard : public Card {
public:
  using Card::Card;
  void applyEffect(DominionState& state, int player) const override;
  static void WitchAttackGiveCurse(DominionState& state, int player);
};

class ThroneRoomCard : public Card {
public:
  using Card::Card;
  void applyEffect(DominionState& state, int player) const override;
  static bool ThroneRoomSelectActionHandler(DominionState& state, int player, Action action_id);
};

const Card& GetCardSpec(CardName name);

std::vector<Action> PendingEffectLegalActions(const DominionState& state, int player);

} // namespace dominion
} // namespace open_spiel

#endif
