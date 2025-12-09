#include <map>
#include <algorithm>

#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

// Card spec registry: base set subset sufficient for tests.
static const std::map<CardName, std::unique_ptr<Card>>& CardRegistry() {
  static std::map<CardName, std::unique_ptr<Card>> reg;
  if (reg.empty()) {
    // Basic supply
    reg.emplace(CardName::CARD_Copper,    std::make_unique<Card>(Card{CardName::CARD_Copper,   "Copper",    {CardType::BASIC_TREASURE}, 0, 1, 0}));
    reg.emplace(CardName::CARD_Silver,    std::make_unique<SilverCard>(SilverCard{CardName::CARD_Silver,   "Silver",    {CardType::BASIC_TREASURE}, 3, 2, 0}));
    reg.emplace(CardName::CARD_Gold,      std::make_unique<Card>(Card{CardName::CARD_Gold,     "Gold",      {CardType::BASIC_TREASURE}, 6, 3, 0}));
    reg.emplace(CardName::CARD_Estate,    std::make_unique<Card>(Card{CardName::CARD_Estate,   "Estate",    {CardType::VICTORY},  2, 0, 1}));
    reg.emplace(CardName::CARD_Duchy,     std::make_unique<Card>(Card{CardName::CARD_Duchy,    "Duchy",     {CardType::VICTORY},  5, 0, 3}));
    reg.emplace(CardName::CARD_Province,  std::make_unique<Card>(Card{CardName::CARD_Province, "Province",  {CardType::VICTORY},  8, 0, 6}));
    reg.emplace(CardName::CARD_Curse,     std::make_unique<Card>(Card{CardName::CARD_Curse,    "Curse",     {CardType::CURSE},    0, 0, -1}));

    // Base set actions
    reg.emplace(CardName::CARD_Village,    std::make_unique<Card>(Card{CardName::CARD_Village,    "Village",   {CardType::ACTION},   3, 0, 0, 2, 1, 0}));
    reg.emplace(CardName::CARD_Smithy,     std::make_unique<Card>(Card{CardName::CARD_Smithy,     "Smithy",    {CardType::ACTION},   4, 0, 0, 0, 3, 0}));
    reg.emplace(CardName::CARD_Market,     std::make_unique<Card>(Card{CardName::CARD_Market,     "Market",    {CardType::ACTION},   5, 1, 0, 1, 1, 1}));
    reg.emplace(CardName::CARD_Laboratory, std::make_unique<Card>(Card{CardName::CARD_Laboratory,  "Laboratory",{CardType::ACTION},   5, 0, 0, 1, 2, 0}));
    reg.emplace(CardName::CARD_Workshop,   std::make_unique<WorkshopCard>(WorkshopCard{CardName::CARD_Workshop,    "Workshop",  {CardType::ACTION},   3, 0, 0, 0, 0, 0, true}));
    reg.emplace(CardName::CARD_Cellar,     std::make_unique<CellarCard>(CellarCard{CardName::CARD_Cellar,      "Cellar",    {CardType::ACTION},   2, 0, 0, 1, 0, 0, true}));
    reg.emplace(CardName::CARD_Chapel,     std::make_unique<ChapelCard>(ChapelCard{CardName::CARD_Chapel,      "Chapel",    {CardType::ACTION},   2, 0, 0, 0, 0, 0, true}));
    reg.emplace(CardName::CARD_Remodel,    std::make_unique<RemodelCard>(RemodelCard{CardName::CARD_Remodel,     "Remodel",   {CardType::ACTION},   4, 0, 0, 0, 0, 0, true}));
    reg.emplace(CardName::CARD_Festival,   std::make_unique<Card>(Card{CardName::CARD_Festival,    "Festival",  {CardType::ACTION},   5, 2, 0, 2, 0, 1}));
    reg.emplace(CardName::CARD_Merchant,   std::make_unique<Card>(Card{CardName::CARD_Merchant,    "Merchant",  {CardType::ACTION},   3, 0, 0, 1, 1, 0}));
    reg.emplace(CardName::CARD_Mine,       std::make_unique<MineCard>(MineCard{CardName::CARD_Mine,        "Mine",      {CardType::ACTION},   5, 0, 0, 0, 0, 0, true}));
    reg.emplace(CardName::CARD_Moat,       std::make_unique<Card>(Card{CardName::CARD_Moat,        "Moat",      {CardType::ACTION},   2, 0, 0, 0, 2, 0}));
    reg.emplace(CardName::CARD_Artisan,    std::make_unique<Card>(Card{CardName::CARD_Artisan,     "Artisan",   {CardType::ACTION},   6, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Militia,    std::make_unique<MilitiaCard>(MilitiaCard{CardName::CARD_Militia,     "Militia",   {CardType::ACTION, CardType::ATTACK},   4, 2, 0, 0, 0, 0, true}));
    reg.emplace(CardName::CARD_Witch,      std::make_unique<WitchCard>(WitchCard{CardName::CARD_Witch,       "Witch",     {CardType::ACTION, CardType::ATTACK},   5, 0, 0, 0, 2, 0, true}));
    reg.emplace(CardName::CARD_Vassal,     std::make_unique<Card>(Card{CardName::CARD_Vassal,      "Vassal",    {CardType::ACTION},   3, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Poacher,    std::make_unique<Card>(Card{CardName::CARD_Poacher,     "Poacher",   {CardType::ACTION},   4, 1, 0, 1, 1, 0}));
    reg.emplace(CardName::CARD_Bandit,     std::make_unique<Card>(Card{CardName::CARD_Bandit,      "Bandit",    {CardType::ACTION},   5, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Bureaucrat, std::make_unique<Card>(Card{CardName::CARD_Bureaucrat,  "Bureaucrat",{CardType::ACTION},   4, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_CouncilRoom,std::make_unique<Card>(Card{CardName::CARD_CouncilRoom, "CouncilRoom",{CardType::ACTION}, 5, 0, 0, 0, 4, 1}));
    reg.emplace(CardName::CARD_Gardens,    std::make_unique<Card>(Card{CardName::CARD_Gardens,     "Gardens",   {CardType::VICTORY},  4, 0, 0}));
    reg.emplace(CardName::CARD_Harbinger,  std::make_unique<Card>(Card{CardName::CARD_Harbinger,   "Harbinger", {CardType::ACTION},   3, 0, 0, 1, 1, 0}));
    reg.emplace(CardName::CARD_Library,    std::make_unique<Card>(Card{CardName::CARD_Library,     "Library",   {CardType::ACTION},   5, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Moneylender,std::make_unique<MoneylenderCard>(MoneylenderCard{CardName::CARD_Moneylender,"Moneylender",{CardType::ACTION}, 4, 0, 0, 0, 0, 0, true}));
    reg.emplace(CardName::CARD_Sentry,     std::make_unique<Card>(Card{CardName::CARD_Sentry,      "Sentry",    {CardType::ACTION},   5, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_ThroneRoom, std::make_unique<ThroneRoomCard>(ThroneRoomCard{CardName::CARD_ThroneRoom,  "ThroneRoom",{CardType::ACTION},   4, 0, 0, 0, 0, 0, true}));
  }
  return reg;
}

// Typed helpers for effect queue management (used locally in this TU).
template <typename T, typename... Args>
static T* PushEffectNode(DominionState& state, int player, Args&&... args) {
  auto ptr = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  T* raw = ptr.get();
  state.player_states_[player].effect_queue.push_back(std::unique_ptr<EffectNode>(std::move(ptr)));
  return raw;
}

template <typename T, typename... Args>
static T* ReplaceFrontEffect(DominionState& state, int player, Args&&... args) {
  auto ptr = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  T* raw = ptr.get();
  if (!state.player_states_[player].effect_queue.empty()) {
    state.player_states_[player].effect_queue.front() = std::unique_ptr<EffectNode>(std::move(ptr));
  } else {
    state.player_states_[player].effect_queue.push_back(std::unique_ptr<EffectNode>(std::move(ptr)));
  }
  return raw;
}

template <typename T>
static T* FrontEffect(DominionState& state, int player) {
  auto& q = state.player_states_[player].effect_queue;
  if (q.empty()) return nullptr;
  return dynamic_cast<T*>(q.front().get());
}

// Returns whether a hand index `j` can be selected under the current front
// effect node. Enforces ascending original-index selection unless a Throne
// Room chain is active, in which case only ACTION cards are selectable.
static bool CanSelectHandIndexForNode(const DominionState& st, int pl,
                                      const EffectNode* node,
                                      int j) {
  if (!node) return false;
  const auto* hs = node->hand_selection();
  if (!hs) return false;
  const auto& p = st.player_states_[pl];
  if (j < 0 || j >= kNumSupplyPiles) return false;
  if (p.hand_counts_[j] <= 0) return false;
  const auto* tnode = dynamic_cast<const ThroneRoomEffectNode*>(node);
  if (tnode && tnode->throne_depth() > 0) {
    const Card& spec = GetCardSpec(static_cast<CardName>(j));
    return std::find(spec.types_.begin(), spec.types_.end(), CardType::ACTION) != spec.types_.end();
  }
  // Optional constraint: only allow treasure selection (used by Mine).
  if (hs->get_only_treasure()) {
    const Card& spec = GetCardSpec(static_cast<CardName>(j));
    if (!spec.IsTreasure()) return false;
  }
  if (node->enforce_ascending && hs->last_selected_original_index_value() >= 0 && j < hs->last_selected_original_index_value()) return false;
  return true;
}

// Generic hand-selection processor: enforces ascending original-index selection
// and handles finish conditions. Per-card effects are provided via callbacks.
// Shared hand-selection processor used by Cellar/Chapel/Militia and parts of
// Throne flows. Applies ascending-index constraint and finish conditions.
bool Card::GenericHandSelectionHandler(
    DominionState& st, int pl, Action action_id,
    bool allow_finish,
    int max_select_count,
    bool finish_on_target_hand_size,
    int select_base,
    Action select_finish,
    const std::function<void(DominionState&, int, int)>& on_select,
    const std::function<void(DominionState&, int)>& on_finish) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::DiscardUpToCardsFromHand &&
      p.pending_choice != PendingChoice::TrashUpToCardsFromHand) return false;
  EffectNode* node = nullptr;
  if (!p.effect_queue.empty()) {
    node = p.effect_queue.front().get();
  }
  SPIEL_CHECK_FALSE(node == nullptr);
  auto* hs = node->hand_selection();
  SPIEL_CHECK_FALSE(hs == nullptr);
  // Optional early finish.
  if (action_id == select_finish) {
    if (!allow_finish) return false;
    if (on_finish) on_finish(st, pl);
    p.ClearDiscardSelection();
    p.pending_choice = PendingChoice::None;
    if (!p.effect_queue.empty()) p.effect_queue.pop_front();
    return true;
  }
  // Handle selecting a hand index.
  if (action_id >= select_base &&
      action_id < select_base + kNumSupplyPiles) {
    int j = static_cast<int>(action_id - select_base);
    SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
    if (CanSelectHandIndexForNode(st, pl, node, j)) {
      on_select(st, pl, j);
      hs->set_last_selected_original_index(j);
      hs->increment_selection_count();
      bool should_finish = false;
      if (max_select_count >= 0 && hs->selection_count_value() >= max_select_count) should_finish = true;
      if (finish_on_target_hand_size && hs->target_hand_size_value() > 0) {
        int hand_sz = 0; for (int k = 0; k < kNumSupplyPiles; ++k) hand_sz += p.hand_counts_[k];
        if (hand_sz <= hs->target_hand_size_value()) should_finish = true;
      }
      if (should_finish) {
        if (on_finish) on_finish(st, pl);
        p.ClearDiscardSelection();
        p.pending_choice = PendingChoice::None;
        if (!p.effect_queue.empty()) p.effect_queue.pop_front();
      }
    }
    return true;
  }
  return false;
}

// Shared helper: handles board gain selection based on node's max_cost.
bool Card::GainFromBoardHandler(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromBoard) return false;
  EffectNode* node = nullptr;
  if (!p.effect_queue.empty()) node = p.effect_queue.front().get();
  SPIEL_CHECK_FALSE(node == nullptr);
  auto* gs = node->gain_from_board();
  SPIEL_CHECK_FALSE(gs == nullptr);
  if (action_id >= ActionIds::GainSelectBase() && action_id < ActionIds::GainSelectBase() + kNumSupplyPiles) {
    int j = static_cast<int>(action_id - ActionIds::GainSelectBase());
    SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
    SPIEL_CHECK_TRUE(st.supply_piles_[j] > 0);
    const Card& spec = GetCardSpec(static_cast<CardName>(j));
    if (spec.cost_ <= gs->max_cost) {
      st.player_states_[pl].discard_counts_[j] += 1;
      st.supply_piles_[j] -= 1;
      p.pending_choice = PendingChoice::None;
      if (!p.effect_queue.empty()) p.effect_queue.pop_front();
      return true;
    }
  }
  return false;
}

const Card& GetCardSpec(CardName name) {
  const auto& reg = CardRegistry();
  return *(reg.at(name));
}

void Card::applyGrants(DominionState& state, int player) const {
  state.actions_ += grant_action_;
  state.buys_ += grant_buy_;
  state.coins_ += value_;
  state.DrawCardsFor(player, grant_draw_);
}

// Default no-op: cards without custom effects rely only on standard grants.
void Card::applyEffect(DominionState&, int) const {}

// Unified play flow for action cards: apply grants then custom effects.
void Card::Play(DominionState& state, int player) const {
  applyGrants(state, player);
  applyEffect(state, player);
}

// Computes legal actions when an effect is pending at the queue front.
// Hand-selection effects expose discard/trash/play actions; gain effects expose
// legal gains filtered by max_cost and supply availability.
std::vector<Action> PendingEffectLegalActions(const DominionState& state, int player) {
  std::vector<Action> actions;
  const auto& ps = state.player_states_[player];
  if (ps.pending_choice == PendingChoice::DiscardUpToCardsFromHand ||
      ps.pending_choice == PendingChoice::TrashUpToCardsFromHand ||
      ps.pending_choice == PendingChoice::PlayActionFromHand) {
    const EffectNode* node = nullptr;
    if (!ps.effect_queue.empty()) node = ps.effect_queue.front().get();
    SPIEL_CHECK_FALSE(node == nullptr);
    const auto* hs = node->hand_selection();
    SPIEL_CHECK_FALSE(hs == nullptr);

    if (ps.pending_choice == PendingChoice::PlayActionFromHand) {
      // Build PlayHandIndex actions with suppression rules for DrawNonTerminal action.
      for (int j = 0; j < kNumSupplyPiles; ++j) {
        if (!CanSelectHandIndexForNode(state, player, node, j)) continue;
        actions.push_back(ActionIds::PlayHandIndex(j));
      }
      actions.push_back(ActionIds::ThroneHandSelectFinish());
    } else {
      bool use_trash = (ps.pending_choice == PendingChoice::TrashUpToCardsFromHand);
      for (int j = 0; j < kNumSupplyPiles; ++j) {
        if (!CanSelectHandIndexForNode(state, player, node, j)) continue;
        actions.push_back(use_trash ? ActionIds::TrashHandSelect(j) : ActionIds::DiscardHandSelect(j));
      }

      if (hs->target_hand_size_value() == 0 || hs->get_allow_finish_selection()) {
          actions.push_back(use_trash ? ActionIds::TrashHandSelectFinish() : ActionIds::DiscardHandSelectFinish());
      }
    }
    // Defensive assertion: during active discard/trash/play effects, legals must not be empty.
    SPIEL_CHECK_FALSE(actions.empty());
  }
  if (ps.pending_choice == PendingChoice::SelectUpToCardsFromBoard) {
    const EffectNode* node = nullptr;
    if (!ps.effect_queue.empty()) node = ps.effect_queue.front().get();
    if (!node) return actions;
    const auto* gs = node->gain_from_board();
    if (!gs) return actions;
    for (int j = 0; j < kNumSupplyPiles; ++j) {
      if (state.supply_piles_[j] <= 0) continue;
      const Card& spec = GetCardSpec(static_cast<CardName>(j));
      if (spec.cost_ <= gs->max_cost) {
        if (gs->get_only_treasure() && !spec.IsTreasure()) continue;
        actions.push_back(ActionIds::GainSelect(j));
      }
    }
  }
  std::sort(actions.begin(), actions.end());
  return actions;
}

// Initializes a hand-selection effect: sets PendingChoice and resets
// effect-local selection counters when available on the node.
void Card::InitHandSelection(DominionState& state, int player, EffectNode* node, PendingChoice choice) {
  auto& ps = state.player_states_[player];
  ps.pending_choice = choice;
  if (node) {
    auto* hs = node->hand_selection();
    if (hs) hs->reset_selection();
  }
}

void Card::InitBoardSelection(DominionState& state, int player) {
  auto& ps = state.player_states_[player];
  ps.pending_choice = PendingChoice::SelectUpToCardsFromBoard;
}

static int AvailableDrawCapacity(const DominionState& st, int pl) {
  const auto& p = st.player_states_[pl];
  int deck_sz = static_cast<int>(p.deck_.size());
  return deck_sz;
}

/* deprecated
void ResolvePlayNonTerminal(DominionState& st, int pl) {
  bool cards_drawn = false; // Allow for reshuffling discard to deck when no cards drawn yet.
  while (st.actions_ >= 1) {
    int capacity = AvailableDrawCapacity(st, pl);
    int best_j = -1;
    int best_draw = -1;
    int best_actions = -1;
    int best_cost = 1e9;
    const auto& p = st.player_states_[pl];
    for (int j = 0; j < kNumSupplyPiles; ++j) {
      if (p.hand_counts_[j] <= 0) continue;
      const Card& spec = GetCardSpec(static_cast<CardName>(j));

      if (std::find(spec.types_.begin(), spec.types_.end(), CardType::ACTION) == spec.types_.end()) continue;
      if (spec.has_unique_effect_ || spec.grant_draw_ == 0) continue;
      if (st.actions_ == 1 && spec.grant_action_ == 0) continue; // Do not play terminal cards if player has 1 action.

      int projected_draw = spec.grant_draw_;
      if (projected_draw > capacity && cards_drawn) continue;
      if (spec.grant_action_ > best_actions ||
          (spec.grant_action_ == best_actions && projected_draw > best_draw) ||
          (spec.grant_action_ == best_actions && projected_draw == best_draw && spec.cost_ < best_cost)) {
        best_j = j;
        best_draw = projected_draw;
        best_actions = spec.grant_action_;
        best_cost = spec.cost_;
      }
    }

    if (best_j < 0) break;
    st.ApplyAction(ActionIds::PlayHandIndex(best_j));
    if (st.IsChanceNode()) {
      SPIEL_CHECK_FALSE(cards_drawn);
      st.ApplyAction(ActionIds::Shuffle());
      continue;
    }
    cards_drawn = true;
    SPIEL_CHECK_FALSE(st.CurrentPlayer() != pl);
    // loop continues with updated state until actions run out or no candidate fits capacity
  }
  SPIEL_CHECK_GE(st.actions_, 1);
}
  */
}  // namespace dominion
}  // namespace open_spiel
