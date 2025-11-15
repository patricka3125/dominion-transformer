#ifndef OPEN_SPIEL_GAMES_DOMINION_CARDS_H_
#define OPEN_SPIEL_GAMES_DOMINION_CARDS_H_

#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "open_spiel/spiel.h"

namespace open_spiel {
namespace dominion {

class DominionState;

class EffectChain;

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
    TREASURE,
    ACTION ,
    VICTORY,
    CURSE 
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
    std::shared_ptr<const EffectChain> effect;
};

class Card {
public:
    std::string name;
    std::vector<CardType> types;
    int cost = 0;
    int value = 0;
    int vp = 0;

    int grant_action = 0; // +Actions
    int grant_draw = 0;   // +Cards
    int grant_buy = 0;    // +Buys

    std::shared_ptr<const EffectChain> effect;

    Card(std::string name_, std::vector<CardType> types_, int cost_=0, int value_=0, int vp_=0,
         int grant_action_ = 0, int grant_draw_ = 0, int grant_buy_ = 0,
         std::shared_ptr<const EffectChain> effect_ = nullptr)
      : name(std::move(name_)),
        types(std::move(types_)),
        cost(cost_), value(value_), vp(vp_),
        grant_action(grant_action_), grant_draw(grant_draw_), grant_buy(grant_buy_),
        effect(std::move(effect_)) {}

    Card(const CardOptions& opt)
      : name(opt.name),
        types(opt.types),
        cost(opt.cost.value_or(0)),
        value(opt.value.value_or(0)),
        vp(opt.vp.value_or(0)),
        grant_action(opt.grant_action.value_or(0)),
        grant_draw(opt.grant_draw.value_or(0)),
        grant_buy(opt.grant_buy.value_or(0)),
        effect(opt.effect) {}

    static Card fromOptions(const CardOptions& opt) { return Card(opt); }

    // Applies standard grants: +actions, +buys, +coins, +cards.
    void play(DominionState& state, int player) const;
    // Triggers any effect chains; used for interactive effects like Cellar.
    void applyEffect(DominionState& state, int player) const;
};

const Card& GetCardSpec(CardName name);

// Reusable card effect helpers
bool HandlePendingEffectAction(DominionState& state, int player, Action action_id);
std::vector<Action> PendingEffectLegalActions(const DominionState& state, int player);

} // namespace dominion
} // namespace open_spiel

#endif