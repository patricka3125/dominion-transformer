#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

class EffectChain;

// Compact storage for card types (optional)
enum class CardType : uint8_t {
    TREASURE = 0,
    ACTION   = 1,
    VICTORY  = 2,
    CURSE    = 3
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
    // Prefer smart pointer; optional is unnecessary for pointers
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

    // Use smart pointer if Card shares/owns effect chains
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
};

// Map type with heterogeneous lookup (string_view/const char* finds are cheap)
struct SvHash {
    using is_transparent = void;
    size_t operator()(std::string_view s) const noexcept { return std::hash<std::string_view>{}(s); }
    size_t operator()(const std::string& s) const noexcept { return std::hash<std::string_view>{}(s); }
    size_t operator()(const char* s) const noexcept { return std::hash<std::string_view>{}(s); }
};
using CardMap = std::unordered_map<std::string, Card, SvHash, std::equal_to<>>;

// Basic Supply Cards
inline const CardMap& BasicCardsMap() {
    static const CardMap basic_cards_dict{
        {"Copper",   {"Copper",   {CardType::TREASURE}, 0, 1}},
        {"Silver",   {"Silver",   {CardType::TREASURE}, 3, 2}},
        {"Gold",     {"Gold",     {CardType::TREASURE}, 6, 3}},
        {"Estate",   {"Estate",   {CardType::VICTORY},  2, 0, 1}},
        {"Duchy",    {"Duchy",    {CardType::VICTORY},  5, 0, 3}},
        {"Province", {"Province", {CardType::VICTORY},  8, 0, 6}}
    };
    return basic_cards_dict;
}

// All cards including basic supply cards
inline const CardMap& CardsMap() {
    static const CardMap cards_dict = [] {
        CardMap m;
        const auto& basic = BasicCardsMap();
        m.reserve(basic.size() + 5);
        m.insert(basic.begin(), basic.end());

        // NOTE: Curse cost often modeled as 0 with negative VP; your -1 sentinel is fine if you handle it.
        m.emplace("Curse",    Card{"Curse",    {CardType::CURSE},   0, 0, -1});
        m.emplace("Village",  Card{"Village",  {CardType::ACTION},  3, 0, 0, 1, 2});
        m.emplace("Smithy",   Card{"Smithy",   {CardType::ACTION},  4, 0, 0, 0, 3});
        m.emplace("Market",   Card{"Market",   {CardType::ACTION},  5, 1, 0, 1, 1, 1});
        m.emplace("Festival", Card{"Festival", {CardType::ACTION},  5, 2, 0, 2});
        return m;
    }();
    return cards_dict;
}