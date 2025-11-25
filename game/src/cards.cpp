#include <map>
#include <algorithm>

#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

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

// Centralized gating for hand index selection based on the current effect node.
// - Enforces ascending original-index selection unless Throne chain is active.
// - Throne chains allow selecting only ACTION cards regardless of index.
static bool CanSelectHandIndexForNode(const DominionState& st, int pl,
                                      const HandSelectionEffectNode* node,
                                      int j) {
  if (!node) return false;
  const auto& p = st.player_states_[pl];
  if (j < 0 || j >= kNumSupplyPiles) return false;
  if (p.hand_counts_[j] <= 0) return false;
  const auto* tnode = dynamic_cast<const ThroneRoomEffectNode*>(node);
  if (tnode && tnode->throne_depth() > 0) {
    const Card& spec = GetCardSpec(static_cast<CardName>(j));
    return std::find(spec.types_.begin(), spec.types_.end(), CardType::ACTION) != spec.types_.end();
  }
  if (node->last_selected_original_index() >= 0 && j < node->last_selected_original_index()) return false;
  return true;
}

// Generic hand-selection processor: enforces ascending original-index selection
// and handles finish conditions. Per-card effects are provided via callbacks.
bool Card::GenericHandSelectionHandler(
    DominionState& st, int pl, Action action_id,
    bool allow_finish,
    int max_select_count,
    bool finish_on_target_hand_size,
    const std::function<void(DominionState&, int, int)>& on_select,
    const std::function<void(DominionState&, int)>& on_finish) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromHand) return false;
  // Access current effect node state
  HandSelectionEffectNode* node = nullptr;
  if (!p.effect_queue.empty()) {
    node = dynamic_cast<HandSelectionEffectNode*>(p.effect_queue.front().get());
  }
  SPIEL_CHECK_FALSE(node == nullptr);
  // Optional early finish.
  if (action_id == ActionIds::HandSelectFinish()) {
    if (!allow_finish) return false;
    if (on_finish) on_finish(st, pl);
    p.ClearDiscardSelection();
    p.pending_choice = PendingChoice::None;
    return true;
  }
  // Handle selecting a hand index.
  if (action_id >= ActionIds::HandSelectBase() &&
      action_id < ActionIds::HandSelectBase() + kNumSupplyPiles) {
    int j = static_cast<int>(action_id - ActionIds::HandSelectBase());
    SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
    if (CanSelectHandIndexForNode(st, pl, node, j)) {
      on_select(st, pl, j);
      node->set_last_selected_original_index(j);
      node->increment_selection_count();
      bool should_finish = false;
      if (max_select_count >= 0 && node->selection_count() >= max_select_count) should_finish = true;
      if (finish_on_target_hand_size && node->target_hand_size() > 0) {
        int hand_sz = 0; for (int k = 0; k < kNumSupplyPiles; ++k) hand_sz += p.hand_counts_[k];
        if (hand_sz <= node->target_hand_size()) should_finish = true;
      }
      if (should_finish) {
        if (on_finish) on_finish(st, pl);
        p.ClearDiscardSelection();
        p.pending_choice = PendingChoice::None;
      }
    }
    return true;
  }
  return false;
}

// Shared helper: handles board gain selection based on pending_gain_max_cost.
bool Card::GainFromBoardHandler(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromBoard) return false;
  GainFromBoardEffectNode* node = FrontEffect<GainFromBoardEffectNode>(st, pl);
  SPIEL_CHECK_FALSE(node == nullptr);
  if (action_id >= ActionIds::GainSelectBase() && action_id < ActionIds::GainSelectBase() + kNumSupplyPiles) {
    int j = static_cast<int>(action_id - ActionIds::GainSelectBase());
    SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
    SPIEL_CHECK_TRUE(st.supply_piles_[j] > 0);
    const Card& spec = GetCardSpec(static_cast<CardName>(j));
    if (spec.cost_ <= node->max_cost()) {
      st.player_states_[pl].discard_counts_[j] += 1;
      st.supply_piles_[j] -= 1;
      p.pending_choice = PendingChoice::None;
      return true;
    }
  }
  return false;
}

// Remodel stage-1 helper: trash one card from hand and switch to board gain.
bool RemodelCard::RemodelTrashFromHand(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromHand) return false;
  if (action_id == ActionIds::HandSelectFinish()) return true;
  HandSelectionEffectNode* node = FrontEffect<HandSelectionEffectNode>(st, pl);
  if (action_id >= ActionIds::HandSelectBase() && action_id < ActionIds::HandSelectBase() + kNumSupplyPiles) {
    int j = static_cast<int>(action_id - ActionIds::HandSelectBase());
    SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
    SPIEL_CHECK_TRUE(p.hand_counts_[j] > 0);
    SPIEL_CHECK_TRUE(node == nullptr || node->last_selected_original_index() < 0 || j >= node->last_selected_original_index());
    const Card& selected = GetCardSpec(static_cast<CardName>(j));
    int cap = selected.cost_ + 2;
    p.hand_counts_[j] -= 1;
    if (node) node->set_last_selected_original_index(j);
    // Switch to board gain stage: replace current front effect with gain-from-board.
    ReplaceFrontEffect<GainFromBoardEffectNode>(st, pl, cap);
    Card::InitBoardSelection(st, pl);
    st.player_states_[pl].effect_queue.front()->on_action = Card::GainFromBoardHandler;
    return true;
  }
  return false;
}

// Cellar handler: select any number of cards in ascending original index; finish draws equal to number discarded.
bool CellarCard::CellarHandSelectHandler(DominionState& st, int pl, Action action_id) {
  // Discard any number; draw equal to discards on finish.
  auto on_select = [](DominionState& st2, int pl2, int j) {
    auto& p2 = st2.player_states_[pl2];
    p2.discard_counts_[j] += 1;
    p2.hand_counts_[j] -= 1;
  };
  auto on_finish = [](DominionState& st2, int pl2) {
    auto& p2 = st2.player_states_[pl2];
    int draw_n = 0;
    if (!p2.effect_queue.empty()) {
      auto* node = dynamic_cast<HandSelectionEffectNode*>(p2.effect_queue.front().get());
      if (node) draw_n = node->selection_count();
    }
    st2.DrawCardsFor(pl2, draw_n);
  };
  return GenericHandSelectionHandler(st, pl, action_id,
                                     /*allow_finish=*/true,
                                     /*max_select_count=*/-1,
                                     /*finish_on_target_hand_size=*/false,
                                     on_select,
                                     on_finish);
}

// Chapel handler: trash up to four cards from hand; finish early allowed.
bool ChapelCard::ChapelHandTrashHandler(DominionState& st, int pl, Action action_id) {
  // Trash up to 4; finish early allowed.
  auto on_select = [](DominionState& st2, int pl2, int j) {
    auto& p2 = st2.player_states_[pl2];
    if (p2.hand_counts_[j] > 0) p2.hand_counts_[j] -= 1;
  };
  auto on_finish = [](DominionState&, int) {};
  return GenericHandSelectionHandler(st, pl, action_id,
                                     /*allow_finish=*/true,
                                     /*max_select_count=*/4,
                                     /*finish_on_target_hand_size=*/false,
                                     on_select,
                                     on_finish);
}

// Militia handler: opponent discards down to 3 cards, then turn returns to attacker.
bool MilitiaCard::MilitiaOpponentDiscardHandler(DominionState& st, int pl, Action action_id) {
  // Opponent discards down to target hand size (3); finish only at threshold and return turn.
  auto on_select = [](DominionState& st2, int pl2, int j) {
    auto& p2 = st2.player_states_[pl2];
    if (p2.hand_counts_[j] > 0) {
      p2.discard_counts_[j] += 1;
      p2.hand_counts_[j] -= 1;
    }
  };
  auto on_finish = [](DominionState& st2, int pl2) {
    st2.current_player_ = 1 - pl2;
  };
  return GenericHandSelectionHandler(st, pl, action_id,
                                     /*allow_finish=*/false,
                                     /*max_select_count=*/-1,
                                     /*finish_on_target_hand_size=*/true,
                                     on_select,
                                     on_finish);
}

// Witch attack effect: each opponent gains a Curse to their discard if available.
void WitchCard::WitchAttackGiveCurse(DominionState& st, int player) {
  int opp = 1 - player;
  int curse_idx = static_cast<int>(CardName::CARD_Curse);
  SPIEL_CHECK_TRUE(curse_idx >= 0 && curse_idx < kNumSupplyPiles);
  if (st.supply_piles_[curse_idx] > 0) {
    st.player_states_[opp].discard_counts_[curse_idx] += 1;
    st.supply_piles_[curse_idx] -= 1;
  }
}

void ThroneRoomEffectNode::BeginSelection(DominionState& state, int player) {
  Card::InitHandSelection(state, player, this);
  state.player_states_[player].effect_queue.front()->on_action = ThroneRoomCard::ThroneRoomSelectActionHandler;
}

void ThroneRoomEffectNode::StartChain(DominionState& state, int player) {
  increment_throne_depth();
  BeginSelection(state, player);
}

void ThroneRoomEffectNode::ContinueOrFinish(DominionState& state, int player) {
  decrement_throne_depth();
  if (throne_depth() == 0) {
    FinishSelection(state, player);
  } else {
    BeginSelection(state, player);
  }
}

void ThroneRoomEffectNode::FinishSelection(DominionState& state, int player) {
  auto& p = state.player_states_[player];
  p.ClearDiscardSelection();
  p.pending_choice = PendingChoice::None;
}

// Throne Room selection: choose one action card from hand; first play executes
// grants and effect without spending an action; second play re-applies effect only.
bool ThroneRoomCard::ThroneRoomSelectActionHandler(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromHand) return false;
  ThroneRoomEffectNode* node = FrontEffect<ThroneRoomEffectNode>(st, pl);
  if (action_id == ActionIds::HandSelectFinish()) {
    // No selection: do nothing, end effect.
    if (node) node->FinishSelection(st, pl);
    return true;
  }
  if (action_id >= ActionIds::HandSelectBase() && action_id < ActionIds::HandSelectBase() + kNumSupplyPiles) {
    int j = static_cast<int>(action_id - ActionIds::HandSelectBase());
    SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
    SPIEL_CHECK_TRUE(p.hand_counts_[j] > 0);
    SPIEL_CHECK_TRUE(node == nullptr || node->last_selected_original_index() < 0 || j >= node->last_selected_original_index());
    CardName cn = static_cast<CardName>(j);
    const Card& spec = GetCardSpec(cn);
    // Must be an action card; ignore non-action selections.
    if (std::find(spec.types_.begin(), spec.types_.end(), CardType::ACTION) == spec.types_.end()) {
      return true;
    }
    // First play: move to play area, do standard grants, no action decrement here.
    p.hand_counts_[j] -= 1;
    st.play_area_.push_back(cn);
    // If the selected card is another Throne Room, chain a new selection node for choosing an action.
    if (cn == CardName::CARD_ThroneRoom) {
      if (node) node->StartChain(st, pl);
    } else {
      // Otherwise, apply the card's own effect twice (Throne effect) and decrement depth once.
      spec.Play(st, pl);
      spec.Play(st, pl);
      if (node) node->ContinueOrFinish(st, pl);
    }
    return true;
  }
  return false;
}
// Card spec registry: base set subset sufficient for tests.
static const std::map<CardName, std::unique_ptr<Card>>& CardRegistry() {
  static std::map<CardName, std::unique_ptr<Card>> reg;
  if (reg.empty()) {
    // Basic supply
    reg.emplace(CardName::CARD_Copper,    std::make_unique<Card>(Card{CardName::CARD_Copper,   "Copper",    {CardType::BASIC_TREASURE}, 0, 1, 0}));
    reg.emplace(CardName::CARD_Silver,    std::make_unique<Card>(Card{CardName::CARD_Silver,   "Silver",    {CardType::BASIC_TREASURE}, 3, 2, 0}));
    reg.emplace(CardName::CARD_Gold,      std::make_unique<Card>(Card{CardName::CARD_Gold,     "Gold",      {CardType::BASIC_TREASURE}, 6, 3, 0}));
    reg.emplace(CardName::CARD_Estate,    std::make_unique<Card>(Card{CardName::CARD_Estate,   "Estate",    {CardType::VICTORY},  2, 0, 1}));
    reg.emplace(CardName::CARD_Duchy,     std::make_unique<Card>(Card{CardName::CARD_Duchy,    "Duchy",     {CardType::VICTORY},  5, 0, 3}));
    reg.emplace(CardName::CARD_Province,  std::make_unique<Card>(Card{CardName::CARD_Province, "Province",  {CardType::VICTORY},  8, 0, 6}));
    reg.emplace(CardName::CARD_Curse,     std::make_unique<Card>(Card{CardName::CARD_Curse,    "Curse",     {CardType::CURSE},    0, 0, -1}));

    // Base set actions
    reg.emplace(CardName::CARD_Village,    std::make_unique<Card>(Card{CardName::CARD_Village,    "Village",   {CardType::ACTION},   3, 0, 0, 2, 1, 0}));
    reg.emplace(CardName::CARD_Smithy,     std::make_unique<Card>(Card{CardName::CARD_Smithy,     "Smithy",    {CardType::ACTION},   4, 0, 0, 0, 3, 0}));
    reg.emplace(CardName::CARD_Market,     std::make_unique<Card>(Card{CardName::CARD_Market,     "Market",    {CardType::ACTION},   5, 1, 0, 1, 1, 1}));
    reg.emplace(CardName::CARD_Laboratory, std::make_unique<Card>(Card{CardName::CARD_Laboratory,  "Laboratory",{CardType::ACTION},   5, 0, 0, 0, 2, 0}));
    reg.emplace(CardName::CARD_Workshop,   std::make_unique<WorkshopCard>(WorkshopCard{CardName::CARD_Workshop,    "Workshop",  {CardType::ACTION},   3, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Cellar,     std::make_unique<CellarCard>(CellarCard{CardName::CARD_Cellar,      "Cellar",    {CardType::ACTION},   2, 0, 0, 1, 0, 0}));
    reg.emplace(CardName::CARD_Chapel,     std::make_unique<ChapelCard>(ChapelCard{CardName::CARD_Chapel,      "Chapel",    {CardType::ACTION},   2, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Remodel,    std::make_unique<RemodelCard>(RemodelCard{CardName::CARD_Remodel,     "Remodel",   {CardType::ACTION},   4, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Festival,   std::make_unique<Card>(Card{CardName::CARD_Festival,    "Festival",  {CardType::ACTION},   5, 2, 0, 2, 0, 1}));
    reg.emplace(CardName::CARD_Merchant,   std::make_unique<Card>(Card{CardName::CARD_Merchant,    "Merchant",  {CardType::ACTION},   3, 0, 0, 1, 1, 0}));
    reg.emplace(CardName::CARD_Mine,       std::make_unique<Card>(Card{CardName::CARD_Mine,        "Mine",      {CardType::ACTION},   5, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Moat,       std::make_unique<Card>(Card{CardName::CARD_Moat,        "Moat",      {CardType::ACTION},   2, 0, 0, 0, 2, 0}));
    reg.emplace(CardName::CARD_Artisan,    std::make_unique<Card>(Card{CardName::CARD_Artisan,     "Artisan",   {CardType::ACTION},   6, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Militia,    std::make_unique<MilitiaCard>(MilitiaCard{CardName::CARD_Militia,     "Militia",   {CardType::ACTION, CardType::ATTACK},   4, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Witch,      std::make_unique<WitchCard>(WitchCard{CardName::CARD_Witch,       "Witch",     {CardType::ACTION, CardType::ATTACK},   5, 0, 0, 0, 2, 0}));
    reg.emplace(CardName::CARD_Vassal,     std::make_unique<Card>(Card{CardName::CARD_Vassal,      "Vassal",    {CardType::ACTION},   3, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Poacher,    std::make_unique<Card>(Card{CardName::CARD_Poacher,     "Poacher",   {CardType::ACTION},   4, 1, 0, 1, 1, 0}));
    reg.emplace(CardName::CARD_Bandit,     std::make_unique<Card>(Card{CardName::CARD_Bandit,      "Bandit",    {CardType::ACTION},   5, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Bureaucrat, std::make_unique<Card>(Card{CardName::CARD_Bureaucrat,  "Bureaucrat",{CardType::ACTION},   4, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_CouncilRoom,std::make_unique<Card>(Card{CardName::CARD_CouncilRoom, "CouncilRoom",{CardType::ACTION}, 5, 0, 0, 0, 4, 1}));
    reg.emplace(CardName::CARD_Gardens,    std::make_unique<Card>(Card{CardName::CARD_Gardens,     "Gardens",   {CardType::VICTORY},  4, 0, 0}));
    reg.emplace(CardName::CARD_Harbinger,  std::make_unique<Card>(Card{CardName::CARD_Harbinger,   "Harbinger", {CardType::ACTION},   3, 0, 0, 1, 1, 0}));
    reg.emplace(CardName::CARD_Library,    std::make_unique<Card>(Card{CardName::CARD_Library,     "Library",   {CardType::ACTION},   5, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Moneylender,std::make_unique<Card>(Card{CardName::CARD_Moneylender,"Moneylender",{CardType::ACTION}, 4, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_Sentry,     std::make_unique<Card>(Card{CardName::CARD_Sentry,      "Sentry",    {CardType::ACTION},   5, 0, 0, 0, 0, 0}));
    reg.emplace(CardName::CARD_ThroneRoom, std::make_unique<ThroneRoomCard>(ThroneRoomCard{CardName::CARD_ThroneRoom,  "ThroneRoom",{CardType::ACTION},   4, 0, 0, 0, 0, 0}));
  }
  return reg;
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

// Derived effect implementations
void CellarCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  auto* node = PushEffectNode<CellarEffectNode>(state, player);
  Card::InitHandSelection(state, player, node);
  ps.effect_queue.front()->on_action = CellarCard::CellarHandSelectHandler;
}

void WorkshopCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  auto* node = PushEffectNode<GainFromBoardEffectNode>(state, player, 4);
  Card::InitBoardSelection(state, player);
  ps.effect_queue.front()->on_action = Card::GainFromBoardHandler;
}

void RemodelCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  auto* node = PushEffectNode<RemodelTrashEffectNode>(state, player);
  Card::InitHandSelection(state, player, node);
  ps.effect_queue.front()->on_action = RemodelCard::RemodelTrashFromHand;
}

void ChapelCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  auto* node = PushEffectNode<ChapelEffectNode>(state, player);
  Card::InitHandSelection(state, player, node);
  ps.effect_queue.front()->on_action = ChapelCard::ChapelHandTrashHandler;
}

void MilitiaCard::applyEffect(DominionState& state, int player) const {
  int opp = 1 - player;
  auto& p_opp = state.player_states_[opp];
  int opp_hand_sz = 0; for (int j=0;j<kNumSupplyPiles;++j) opp_hand_sz += p_opp.hand_counts_[j];
  if (opp_hand_sz > 3) {
    p_opp.effect_queue.clear();
    auto* n = PushEffectNode<MilitiaEffectNode>(state, opp);
    n->set_target_hand_size(3);
    Card::InitHandSelection(state, opp, n);
    p_opp.effect_queue.front()->on_action = MilitiaCard::MilitiaOpponentDiscardHandler;
    state.current_player_ = opp;
  }
}

void WitchCard::applyEffect(DominionState& state, int player) const {
  WitchCard::WitchAttackGiveCurse(state, player);
}

void ThroneRoomCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  ps.effect_queue.clear();
  auto* node = PushEffectNode<ThroneRoomEffectNode>(state, player);
  node->StartChain(state, player);
}


std::vector<Action> PendingEffectLegalActions(const DominionState& state, int player) {
  std::vector<Action> actions;
  const auto& ps = state.player_states_[player];
  if (ps.pending_choice == PendingChoice::SelectUpToCardsFromHand) {
    const HandSelectionEffectNode* node = nullptr;
    if (!ps.effect_queue.empty()) node = dynamic_cast<const HandSelectionEffectNode*>(ps.effect_queue.front().get());
    if (!node) return actions;
    for (int j = 0; j < kNumSupplyPiles; ++j) {
      if (CanSelectHandIndexForNode(state, player, node, j)) {
        actions.push_back(ActionIds::HandSelect(j));
      }
    }
    if (node->target_hand_size() == 0) {
      actions.push_back(ActionIds::HandSelectFinish());
    }
  }
  if (ps.pending_choice == PendingChoice::SelectUpToCardsFromBoard) {
    const GainFromBoardEffectNode* node = nullptr;
    if (!ps.effect_queue.empty()) node = dynamic_cast<const GainFromBoardEffectNode*>(ps.effect_queue.front().get());
    if (!node) return actions;
    for (int j = 0; j < kNumSupplyPiles; ++j) {
      if (state.supply_piles_[j] <= 0) continue;
      const Card& spec = GetCardSpec(static_cast<CardName>(j));
      if (spec.cost_ <= node->max_cost()) {
        actions.push_back(ActionIds::GainSelect(j));
      }
    }
  }
  std::sort(actions.begin(), actions.end());
  return actions;
}

void Card::InitHandSelection(DominionState& state, int player, HandSelectionEffectNode* node) {
  auto& ps = state.player_states_[player];
  ps.pending_choice = PendingChoice::SelectUpToCardsFromHand;
  if (node) node->reset_selection();
}

void Card::InitBoardSelection(DominionState& state, int player) {
  auto& ps = state.player_states_[player];
  ps.pending_choice = PendingChoice::SelectUpToCardsFromBoard;
}

}  // namespace dominion
}  // namespace open_spiel