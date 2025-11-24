#include <map>
#include <algorithm>

#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

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
  SelectUpToCardsNode* node = nullptr;
  if (!p.effect_queue.empty()) {
    node = dynamic_cast<SelectUpToCardsNode*>(p.effect_queue.front().get());
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
    SPIEL_CHECK_TRUE(p.hand_counts_[j] > 0);
    if (j >= 0 && j < kNumSupplyPiles && p.hand_counts_[j] > 0) {
      // Enforce ascending enumerator-based selection.
      SPIEL_CHECK_TRUE(node->last_selected_original_index() < 0 || j >= node->last_selected_original_index());
      if (node->last_selected_original_index() < 0 || j >= node->last_selected_original_index()) {
        on_select(st, pl, j);
        node->set_last_selected_original_index(j);
        node->increment_discard_count();
        bool should_finish = false;
        if (max_select_count >= 0 && node->discard_count() >= max_select_count) should_finish = true;
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
    }
    return true;
  }
  return false;
}

// Shared helper: handles board gain selection based on pending_gain_max_cost.
bool Card::GainFromBoardHandler(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromBoard) return false;
  SelectUpToCardsFromBoardNode* node = nullptr;
  if (!p.effect_queue.empty()) {
    node = dynamic_cast<SelectUpToCardsFromBoardNode*>(p.effect_queue.front().get());
  }
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
bool Card::RemodelTrashFromHand(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromHand) return false;
  if (action_id == ActionIds::HandSelectFinish()) return true;
  SelectUpToCardsNode* node = nullptr;
  if (!p.effect_queue.empty()) {
    node = dynamic_cast<SelectUpToCardsNode*>(p.effect_queue.front().get());
  }
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
    if (!st.player_states_[pl].effect_queue.empty()) {
      st.player_states_[pl].effect_queue.front() = std::unique_ptr<EffectNode>(new SelectUpToCardsFromBoardNode(cap));
      st.player_states_[pl].effect_queue.front()->onEnter(st, pl);
      st.player_states_[pl].effect_queue.front()->on_action = GainFromBoardHandler;
    }
    return true;
  }
  return false;
}

// Cellar handler: select any number of cards in ascending original index; finish draws equal to number discarded.
bool Card::CellarHandSelectHandler(DominionState& st, int pl, Action action_id) {
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
      auto* node = dynamic_cast<SelectUpToCardsNode*>(p2.effect_queue.front().get());
      if (node && node->draw_equals_discard()) draw_n = node->discard_count();
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
bool Card::ChapelHandTrashHandler(DominionState& st, int pl, Action action_id) {
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
bool Card::MilitiaOpponentDiscardHandler(DominionState& st, int pl, Action action_id) {
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
void Card::WitchAttackGiveCurse(DominionState& st, int player) {
  int opp = 1 - player;
  int curse_idx = static_cast<int>(CardName::CARD_Curse);
  SPIEL_CHECK_TRUE(curse_idx >= 0 && curse_idx < kNumSupplyPiles);
  if (st.supply_piles_[curse_idx] > 0) {
    st.player_states_[opp].discard_counts_[curse_idx] += 1;
    st.supply_piles_[curse_idx] -= 1;
  }
}

// Throne Room selection: choose one action card from hand; first play executes
// grants and effect without spending an action; second play re-applies effect only.
bool Card::ThroneRoomSelectActionHandler(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromHand) return false;
  SelectUpToCardsNode* node = nullptr;
  if (!p.effect_queue.empty()) node = dynamic_cast<SelectUpToCardsNode*>(p.effect_queue.front().get());
  if (action_id == ActionIds::HandSelectFinish()) {
    // No selection: do nothing, end effect.
    p.ClearDiscardSelection();
    p.pending_choice = PendingChoice::None;
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
    spec.play(st, pl);
    // If the selected card is another Throne Room, chain a new selection node for choosing an action.
    if (cn == CardName::CARD_ThroneRoom) {
      if (node) node->increment_throne_depth();
      // Continue selection on the same node; reinitialize choice state.
      if (!st.player_states_[pl].effect_queue.empty()) {
        st.player_states_[pl].effect_queue.front()->onEnter(st, pl);
        st.player_states_[pl].effect_queue.front()->on_action = ThroneRoomSelectActionHandler;
      }
    } else {
      // Otherwise, apply the card's own effect twice (Throne effect) and decrement depth once.
      spec.applyEffect(st, pl);
      spec.play(st, pl);
      spec.applyEffect(st, pl);
      if (node) node->decrement_throne_depth();
      if (!node || node->throne_depth() == 0) {
        // End Throne Room selection effect when depth satisfied.
        p.ClearDiscardSelection();
        p.pending_choice = PendingChoice::None;
      } else {
        // Continue selection; keep choosing actions.
        if (!st.player_states_[pl].effect_queue.empty()) {
          st.player_states_[pl].effect_queue.front() = std::unique_ptr<EffectNode>(new SelectUpToCardsNode(false));
          st.player_states_[pl].effect_queue.front()->onEnter(st, pl);
          st.player_states_[pl].effect_queue.front()->on_action = ThroneRoomSelectActionHandler;
        }
      }
    }
    return true;
  }
  return false;
}
// Card spec registry: base set subset sufficient for tests.
static const std::map<CardName, Card>& CardRegistry() {
  static std::map<CardName, Card> reg = {
    {CardName::CARD_Copper,    Card{CardName::CARD_Copper,   "Copper",    {CardType::BASIC_TREASURE}, 0, 1, 0}},
    {CardName::CARD_Silver,    Card{CardName::CARD_Silver,   "Silver",    {CardType::BASIC_TREASURE}, 3, 2, 0}},
    {CardName::CARD_Gold,      Card{CardName::CARD_Gold,     "Gold",      {CardType::BASIC_TREASURE}, 6, 3, 0}},
    {CardName::CARD_Estate,    Card{CardName::CARD_Estate,   "Estate",    {CardType::VICTORY},  2, 0, 1}},
    {CardName::CARD_Duchy,     Card{CardName::CARD_Duchy,    "Duchy",     {CardType::VICTORY},  5, 0, 3}},
    {CardName::CARD_Province,  Card{CardName::CARD_Province, "Province",  {CardType::VICTORY},  8, 0, 6}},
    {CardName::CARD_Curse,     Card{CardName::CARD_Curse,    "Curse",     {CardType::CURSE},    0, 0, -1}},

    {CardName::CARD_Village,   Card{CardName::CARD_Village,    "Village",   {CardType::ACTION},   3, 0, 0, 2, 1, 0}},
    {CardName::CARD_Smithy,    Card{CardName::CARD_Smithy,     "Smithy",    {CardType::ACTION},   4, 0, 0, 0, 3, 0}},
    {CardName::CARD_Market,    Card{CardName::CARD_Market,     "Market",    {CardType::ACTION},   5, 1, 0, 1, 1, 1}},
    {CardName::CARD_Laboratory,Card{CardName::CARD_Laboratory,  "Laboratory",{CardType::ACTION},   5, 0, 0, 0, 2, 0}},
    {CardName::CARD_Workshop,  Card{CardName::CARD_Workshop,    "Workshop",  {CardType::ACTION},   3, 0, 0, 0, 0, 0}},
    {CardName::CARD_Cellar,    Card{CardName::CARD_Cellar,      "Cellar",    {CardType::ACTION},   2, 0, 0, 1, 0, 0}},
    {CardName::CARD_Chapel,    Card{CardName::CARD_Chapel,      "Chapel",    {CardType::ACTION},   2, 0, 0, 0, 0, 0}},
    {CardName::CARD_Remodel,   Card{CardName::CARD_Remodel,     "Remodel",   {CardType::ACTION},   4, 0, 0, 0, 0, 0}},
    {CardName::CARD_Festival,  Card{CardName::CARD_Festival,    "Festival",  {CardType::ACTION},   5, 2, 0, 2, 0, 1}},
    {CardName::CARD_Merchant,  Card{CardName::CARD_Merchant,    "Merchant",  {CardType::ACTION},   3, 0, 0, 1, 1, 0}},
    {CardName::CARD_Mine,      Card{CardName::CARD_Mine,        "Mine",      {CardType::ACTION},   5, 0, 0, 0, 0, 0}},
    {CardName::CARD_Moat,      Card{CardName::CARD_Moat,        "Moat",      {CardType::ACTION},   2, 0, 0, 0, 2, 0}},
    {CardName::CARD_Artisan,   Card{CardName::CARD_Artisan,     "Artisan",   {CardType::ACTION},   6, 0, 0, 0, 0, 0}},
    {CardName::CARD_Militia,   Card{CardName::CARD_Militia,     "Militia",   {CardType::ACTION, CardType::ATTACK},   4, 0, 0, 0, 0, 0}},
    {CardName::CARD_Witch,     Card{CardName::CARD_Witch,       "Witch",     {CardType::ACTION, CardType::ATTACK},   5, 0, 0, 0, 2, 0}},
    {CardName::CARD_Vassal,    Card{CardName::CARD_Vassal,      "Vassal",    {CardType::ACTION},   3, 0, 0, 0, 0, 0}},
    {CardName::CARD_Poacher,   Card{CardName::CARD_Poacher,     "Poacher",   {CardType::ACTION},   4, 1, 0, 1, 1, 0}},
    {CardName::CARD_Bandit,    Card{CardName::CARD_Bandit,      "Bandit",    {CardType::ACTION},   5, 0, 0, 0, 0, 0}},
    {CardName::CARD_Bureaucrat,Card{CardName::CARD_Bureaucrat,  "Bureaucrat",{CardType::ACTION},   4, 0, 0, 0, 0, 0}},
    {CardName::CARD_CouncilRoom,Card{CardName::CARD_CouncilRoom, "CouncilRoom",{CardType::ACTION}, 5, 0, 0, 0, 4, 1}},
    {CardName::CARD_Gardens,   Card{CardName::CARD_Gardens,     "Gardens",   {CardType::VICTORY},  4, 0, 0}},
    {CardName::CARD_Harbinger, Card{CardName::CARD_Harbinger,   "Harbinger", {CardType::ACTION},   3, 0, 0, 1, 1, 0}},
    {CardName::CARD_Library,   Card{CardName::CARD_Library,     "Library",   {CardType::ACTION},   5, 0, 0, 0, 0, 0}},
    {CardName::CARD_Moneylender,Card{CardName::CARD_Moneylender,"Moneylender",{CardType::ACTION}, 4, 0, 0, 0, 0, 0}},
    {CardName::CARD_Sentry,    Card{CardName::CARD_Sentry,      "Sentry",    {CardType::ACTION},   5, 0, 0, 0, 0, 0}},
    {CardName::CARD_ThroneRoom,Card{CardName::CARD_ThroneRoom,  "ThroneRoom",{CardType::ACTION},   4, 0, 0, 0, 0, 0}},
  };
  return reg;
}

const Card& GetCardSpec(CardName name) {
  return CardRegistry().at(name);
}

void Card::play(DominionState& state, int player) const {
  state.actions_ += grant_action_;
  state.buys_ += grant_buy_;
  state.coins_ += value_;
  state.DrawCardsFor(player, grant_draw_);
}

void Card::applyEffect(DominionState& state, int player) const {
  if (kind_ == CardName::CARD_Cellar) {
    auto& ps = state.player_states_[player];
    ps.effect_queue.clear();
    ps.effect_queue.push_back(std::unique_ptr<EffectNode>(new SelectUpToCardsNode(true)));
    ps.effect_queue.front()->onEnter(state, player);
    ps.effect_queue.front()->on_action = CellarHandSelectHandler;
  }
  if (kind_ == CardName::CARD_Workshop) {
    auto& ps = state.player_states_[player];
    ps.effect_queue.clear();
    ps.effect_queue.push_back(std::unique_ptr<EffectNode>(new SelectUpToCardsFromBoardNode(4)));
    ps.effect_queue.front()->onEnter(state, player);
    // Reuse shared handler for gain-from-board.
    ps.effect_queue.front()->on_action = GainFromBoardHandler;
  }
  if (kind_ == CardName::CARD_Remodel) {
    auto& ps = state.player_states_[player];
    // First stage: choose exactly one card from hand to trash (remove from game).
    ps.effect_queue.clear();
    ps.effect_queue.push_back(std::unique_ptr<EffectNode>(new SelectUpToCardsNode(false)));
    ps.effect_queue.front()->onEnter(state, player);
    ps.effect_queue.front()->on_action = RemodelTrashFromHand;
  }
  if (kind_ == CardName::CARD_Chapel) {
    auto& ps = state.player_states_[player];
    ps.effect_queue.clear();
    ps.effect_queue.push_back(std::unique_ptr<EffectNode>(new SelectUpToCardsNode(false)));
    ps.effect_queue.front()->onEnter(state, player);
    ps.effect_queue.front()->on_action = ChapelHandTrashHandler;
  }
  if (kind_ == CardName::CARD_Militia) {
    int opp = 1 - player;
    auto& p_opp = state.player_states_[opp];
    int opp_hand_sz = 0; for (int j=0;j<kNumSupplyPiles;++j) opp_hand_sz += p_opp.hand_counts_[j];
    if (opp_hand_sz > 3) {
      p_opp.effect_queue.clear();
      auto n = std::unique_ptr<SelectUpToCardsNode>(new SelectUpToCardsNode(false));
      n->set_target_hand_size(3);
      p_opp.effect_queue.push_back(std::unique_ptr<EffectNode>(std::move(n)));
      p_opp.effect_queue.front()->onEnter(state, opp);
      p_opp.effect_queue.front()->on_action = MilitiaOpponentDiscardHandler;
      state.current_player_ = opp;
    }
  }
  if (kind_ == CardName::CARD_Witch) {
    WitchAttackGiveCurse(state, player);
  }
  if (kind_ == CardName::CARD_ThroneRoom) {
    auto& ps = state.player_states_[player];
    ps.effect_queue.clear();
    auto n = std::unique_ptr<SelectUpToCardsNode>(new SelectUpToCardsNode(false));
    n->increment_throne_depth();
    ps.effect_queue.push_back(std::unique_ptr<EffectNode>(std::move(n)));
    ps.effect_queue.front()->onEnter(state, player);
    ps.effect_queue.front()->on_action = ThroneRoomSelectActionHandler;
  }
}


std::vector<Action> PendingEffectLegalActions(const DominionState& state, int player) {
  std::vector<Action> actions;
  const auto& ps = state.player_states_[player];
  if (ps.pending_choice == PendingChoice::SelectUpToCardsFromHand) {
    const SelectUpToCardsNode* node = nullptr;
    if (!ps.effect_queue.empty()) node = dynamic_cast<const SelectUpToCardsNode*>(ps.effect_queue.front().get());
    if (!node) return actions;
    for (int j = 0; j < kNumSupplyPiles; ++j) {
      if (ps.hand_counts_[j] <= 0) continue;
      SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
      if (node->last_selected_original_index() >= 0 && j < node->last_selected_original_index()) continue;
      if (node->throne_depth() <= 0) {
        actions.push_back(ActionIds::HandSelect(j));
      } else {
        const Card& spec = GetCardSpec(static_cast<CardName>(j));
        if (std::find(spec.types_.begin(), spec.types_.end(), CardType::ACTION) != spec.types_.end()) {
          actions.push_back(ActionIds::HandSelect(j));
        }
      }
    }
    if (node->target_hand_size() == 0) {
      actions.push_back(ActionIds::HandSelectFinish());
    }
  }
  if (ps.pending_choice == PendingChoice::SelectUpToCardsFromBoard) {
    const SelectUpToCardsFromBoardNode* node = nullptr;
    if (!ps.effect_queue.empty()) node = dynamic_cast<const SelectUpToCardsFromBoardNode*>(ps.effect_queue.front().get());
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

}  // namespace dominion
}  // namespace open_spiel
namespace {
}

namespace open_spiel { namespace dominion {
void SelectUpToCardsNode::onEnter(DominionState& state, int player) {
  auto& ps = state.player_states_[player];
  ps.pending_choice = PendingChoice::SelectUpToCardsFromHand;
  // Initialize node-local selection metadata
  last_selected_original_index_ = -1;
  discard_count_ = 0;
}
void SelectUpToCardsFromBoardNode::onEnter(DominionState& state, int player) {
  auto& ps = state.player_states_[player];
  ps.pending_choice = PendingChoice::SelectUpToCardsFromBoard;
}
} }
