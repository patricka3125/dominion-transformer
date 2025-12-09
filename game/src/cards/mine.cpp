#include "cards.hpp"
#include "dominion.hpp"
#include "actions.hpp"

namespace open_spiel {
namespace dominion {

bool MineCard::MineTrashFromHand(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::TrashUpToCardsFromHand) return false;
  EffectNode* node = p.FrontEffect();
  if (action_id >= ActionIds::TrashHandBase() && action_id < ActionIds::TrashHandBase() + kNumSupplyPiles) {
    int j = static_cast<int>(action_id - ActionIds::TrashHandBase());
    SPIEL_CHECK_TRUE(IsValidPileIndex(j));
    SPIEL_CHECK_TRUE(p.hand_counts_[j] > 0);
    // Ascending selection is not enforced for trash; allow any index present in hand.
    const auto* hs = node ? node->hand_selection() : nullptr;
    const Card& selected = GetCardSpec(ToCardName(j));
    // Only allow trashing treasures for Mine effect
    SPIEL_CHECK_TRUE(selected.IsTreasure());
    int cap = selected.cost_ + 3;
    // Trash selection: remove from hand.
    p.hand_counts_[j] -= 1;
    if (hs) const_cast<HandSelectionStruct*>(hs)->set_last_selected_original_index(j);
    // Switch to board gain stage: replace current front effect with gain-from-board.
    auto n = EffectNodeFactory::CreateGainEffect(CardName::CARD_Mine, cap);
    p.effect_queue.front() = std::move(n);
    Card::InitBoardSelection(st, pl);
    p.FrontEffect()->on_action = MineCard::MineGainFromBoardHandler;
    return true;
  }
  return false;
}

bool MineCard::MineGainFromBoardHandler(DominionState& st, int pl, Action action_id) {
  auto& p = st.player_states_[pl];
  if (p.pending_choice != PendingChoice::SelectUpToCardsFromBoard) return false;
  EffectNode* node = p.FrontEffect();
  SPIEL_CHECK_FALSE(node == nullptr);
  auto* gs = node->gain_from_board();
  SPIEL_CHECK_FALSE(gs == nullptr);
  if (action_id >= ActionIds::GainSelectBase() && action_id < ActionIds::GainSelectBase() + kNumSupplyPiles) {
    int j = static_cast<int>(action_id - ActionIds::GainSelectBase());
    SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
    SPIEL_CHECK_TRUE(st.supply_piles_[j] > 0);
    const Card& spec = GetCardSpec(static_cast<CardName>(j));
    // Gain only treasure up to max_cost
    SPIEL_CHECK_TRUE(spec.IsTreasure());
    if (spec.cost_ <= gs->max_cost) {
      st.player_states_[pl].hand_counts_[j] += 1;
      st.supply_piles_[j] -= 1;
      p.pending_choice = PendingChoice::None;
      if (!p.effect_queue.empty()) p.effect_queue.pop_front();
      return true;
    }
  }
  return false;
}

void MineCard::applyEffect(DominionState& state, int player) const {
  auto& ps = state.player_states_[player];
  if (ps.TotalHandSize() == 0) return;
  ps.effect_queue.clear();
  auto n = EffectNodeFactory::CreateHandSelectionEffect(
      CardName::CARD_Mine,
      PendingChoice::TrashUpToCardsFromHand);
  ps.effect_queue.push_back(std::move(n));
  Card::InitHandSelection(state, player, ps.FrontEffect(), PendingChoice::TrashUpToCardsFromHand);
  if (auto* hs = ps.FrontEffect()->hand_selection()) {
    hs->set_only_treasure();
  }
  ps.FrontEffect()->on_action = MineCard::MineTrashFromHand;
}

} // namespace dominion
} // namespace open_spiel
