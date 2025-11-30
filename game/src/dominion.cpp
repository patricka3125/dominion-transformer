#include <algorithm>
#include <cstdlib>
#include <map>
#include <random>

#include "actions.hpp"
#include "cards.hpp"
#include "dominion.hpp"
#include "open_spiel/spiel.h"

namespace open_spiel {
namespace dominion {

using nlohmann::json;

namespace {

const GameType kGameType{
    "dominion",
    "Dominion (Base)",
    GameType::Dynamics::kSequential,
    GameType::ChanceMode::kSampledStochastic,
    GameType::Information::kImperfectInformation,
    GameType::Utility::kZeroSum,
    GameType::RewardModel::kTerminal,
    /*max_num_players=*/kNumPlayers,
    /*min_num_players=*/kNumPlayers,
    /*provides_information_state_string=*/true,
    /*provides_information_state_tensor=*/false,
    /*provides_observation_string=*/true,
    /*provides_observation_tensor=*/false,
    /*parameter_specification=*/{}};

std::shared_ptr<const Game> Factory(const GameParameters &params) {
  return std::shared_ptr<const Game>(new DominionGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

} // namespace

DominionGame::DominionGame(const GameParameters &params)
    : Game(kGameType, params) {
  // No game-level RNG management; chance events use local RNG.
}

int DominionGame::NumDistinctActions() const {
  return kDominionMaxDistinctActions;
}

std::unique_ptr<State> DominionGame::NewInitialState() const {
  return std::unique_ptr<State>(new DominionState(shared_from_this()));
}

int DominionGame::NumPlayers() const { return kNumPlayers; }

double DominionGame::MinUtility() const { return -50.0; }

double DominionGame::MaxUtility() const { return 50.0; }

std::vector<int> DominionGame::InformationStateTensorShape() const {
  return {};
}

std::vector<int> DominionGame::ObservationTensorShape() const { return {}; }

int DominionGame::MaxGameLength() const { return 500; }

int DominionGame::MaxChanceOutcomes() const { return 1; }

std::unique_ptr<State> DominionGame::NewInitialState(const json &j) const {
  return std::unique_ptr<State>(new DominionState(shared_from_this(), j));
}

std::unique_ptr<State> DominionGame::DeserializeState(const std::string &str) const {
  json j = json::parse(str);
  return std::unique_ptr<State>(new DominionState(shared_from_this(), j));
}

namespace {
static bool HasType(const Card &c, CardType t) {
  return std::find(c.types_.begin(), c.types_.end(), t) != c.types_.end();
}

// Format an action as "id:name" using the game's action-naming helpers.
static std::string FormatActionPair(const DominionState& st, Action a) {
  return std::to_string(static_cast<int>(a)) + ":" + st.ActionToString(st.CurrentPlayer(), a);
}
} // namespace

void DominionState::DrawCardsFor(int player, int n) {
  auto &ps = player_states_[player];
  for (int i = 0; i < n; ++i) {
    if (ps.deck_.empty()) {
      int discard_size = 0; for (int jj=0;jj<kNumSupplyPiles;++jj) discard_size += ps.discard_counts_[jj];
      if (discard_size == 0) break;
      shuffle_pending_ = true;
      original_player_for_shuffle_ = player;
      pending_draw_count_after_shuffle_ = n - i;
      return;
    }
    int idx = static_cast<int>(ps.deck_.back());
    if (idx >= 0 && idx < kNumSupplyPiles) {
      ps.hand_counts_[idx] += 1;
    }
    ps.deck_.pop_back();
  }
}

DominionState::DominionState(std::shared_ptr<const Game> game) : State(game) {
  // Shuffle initial decks using a local RNG to introduce chance.
  unsigned seed = static_cast<unsigned>(std::random_device{}());
  std::mt19937 rng(seed);

  // Initialize supply piles indexed by CardName; non-board cards are 0.
  for (int j = 0; j < kNumSupplyPiles; ++j) supply_piles_[j] = 0;
  supply_piles_[static_cast<int>(CardName::CARD_Copper)] = 60;
  supply_piles_[static_cast<int>(CardName::CARD_Silver)] = 40;
  supply_piles_[static_cast<int>(CardName::CARD_Gold)] = 30;
  supply_piles_[static_cast<int>(CardName::CARD_Estate)] = 8;
  supply_piles_[static_cast<int>(CardName::CARD_Duchy)] = 8;
  supply_piles_[static_cast<int>(CardName::CARD_Province)] = 8;
  supply_piles_[static_cast<int>(CardName::CARD_Curse)] = 10;
  std::array<CardName, 10> kingdom = {CardName::CARD_Cellar,  CardName::CARD_Market,  CardName::CARD_Militia,
                                      CardName::CARD_Laboratory,    CardName::CARD_Moat,    CardName::CARD_Remodel,
                                      CardName::CARD_Smithy,  CardName::CARD_Village, CardName::CARD_Workshop,
                                      CardName::CARD_Festival};
  for (CardName cn : kingdom) supply_piles_[static_cast<int>(cn)] = 10;
  initial_supply_piles_ = supply_piles_;

  // Initial decks and hands
  for (int p = 0; p < kNumPlayers; ++p) {
    auto &ps = player_states_[p];
    ps.deck_.clear();
    ps.discard_counts_.fill(0);
    ps.hand_counts_.fill(0);
    ps.obs_state =
        std::make_unique<ObservationState>(ps.hand_counts_, ps.deck_, ps.discard_counts_);
    for (int i = 0; i < 7; ++i) {
      ps.deck_.push_back(CardName::CARD_Copper);
    }
    for (int i = 0; i < 3; ++i) {
      ps.deck_.push_back(CardName::CARD_Estate);
    }
    // Shuffle the 10-card starting deck using local RNG.
    std::shuffle(ps.deck_.begin(), ps.deck_.end(), rng);

    DrawCardsFor(p, 5);
  }

  current_player_ = 0;
  actions_ = 1;
  buys_ = 1;
  coins_ = 0;
  phase_ = Phase::actionPhase;
  // Optimization: if the starting hand has no playable actions, begin in buy phase.
  MaybeAutoAdvanceToBuyPhase();
}

DominionState::DominionState(std::shared_ptr<const Game> game, const json &j)
    : State(game) {
  DominionStateStructContents contents = j.get<DominionStateStructContents>();
  current_player_ = contents.current_player;
  coins_ = contents.coins;
  turn_number_ = contents.turn_number;
  actions_ = contents.actions;
  buys_ = contents.buys;
  phase_ = static_cast<Phase>(contents.phase);
  last_player_to_go_ = contents.last_player_to_go;
  shuffle_pending_ = contents.shuffle_pending;
  shuffle_pending_end_of_turn_ = contents.shuffle_pending_end_of_turn;
  original_player_for_shuffle_ = contents.original_player_for_shuffle;
  pending_draw_count_after_shuffle_ = contents.pending_draw_count_after_shuffle;
  supply_piles_ = contents.supply_piles;
  initial_supply_piles_ = contents.initial_supply_piles;
  play_area_.clear();
  for (int v : contents.play_area) play_area_.push_back(static_cast<CardName>(v));
  for (int p = 0; p < kNumPlayers; ++p) {
    if (p < static_cast<int>(contents.player_states.size())) {
      json pj = contents.player_states[p];
      DominionPlayerStateStruct ss(pj.dump());
      player_states_[p].LoadFromStruct(ss);
    } else {
      // Leave as default-initialized; ensure obs_state is set.
      player_states_[p].obs_state = std::make_unique<ObservationState>(player_states_[p].hand_counts_, player_states_[p].deck_, player_states_[p].discard_counts_);
    }
  }
  history_.clear();
  move_number_ = contents.move_number;
}

Player DominionState::CurrentPlayer() const { return shuffle_pending_ ? kChancePlayerId : current_player_; }

// Computes the legal actions for the current player.
// Returns sorted IDs and delegates to pending-effect logic first.
std::vector<Action> DominionState::LegalActions() const {
  std::vector<Action> actions;
  if (IsTerminal())
    return actions;
  if (IsChanceNode()) {
    actions.push_back(ActionIds::Shuffle());
    return actions;
  }
  const auto &ps = player_states_[current_player_];
  {
    auto pend = PendingEffectLegalActions(*this, current_player_);
    if (!pend.empty())
      return pend;
  }
  if (phase_ == Phase::actionPhase) {
    if (actions_ > 0) {
      for (int j = 0; j < kNumSupplyPiles; ++j) {
        if (ps.hand_counts_[j] <= 0) continue;
        const Card &spec = GetCardSpec(static_cast<CardName>(j));
        if (HasType(spec, CardType::ACTION)) {
          actions.push_back(ActionIds::PlayHandIndex(j));
        }
      }
    }
    actions.push_back(ActionIds::EndActions());
  } else if (phase_ == Phase::buyPhase) {
    int effective_coins = coins_;
    for (int j = 0; j < kNumSupplyPiles; ++j) {
      if (ps.hand_counts_[j] <= 0) continue;
      const Card &spec = GetCardSpec(static_cast<CardName>(j));
      if (HasType(spec, CardType::BASIC_TREASURE)) {
        effective_coins += ps.hand_counts_[j] * spec.value_;
        continue;
      }
      if (HasType(spec, CardType::SPECIAL_TREASURE)) {
        actions.push_back(ActionIds::PlayHandIndex(j));
      }
    }
    if (buys_ > 0) {
      for (int j = 0; j < kNumSupplyPiles; ++j) {
        if (supply_piles_[j] <= 0) continue;
        const Card &spec = GetCardSpec(static_cast<CardName>(j));
        if (effective_coins >= spec.cost_) actions.push_back(ActionIds::BuyFromSupply(j));
      }
    }
    actions.push_back(ActionIds::EndBuy());
  }
  std::sort(actions.begin(), actions.end());
  return actions;
}

std::string DominionState::ActionToString(Player player,
                                          Action action_id) const {
  return ActionNames::NameWithCard(action_id, kNumSupplyPiles);
}

std::unique_ptr<StateStruct> DominionState::ToStruct() const {
  DominionStateStructContents contents;
  contents.current_player = current_player_;
  contents.coins = coins_;
  contents.turn_number = turn_number_;
  contents.actions = actions_;
  contents.buys = buys_;
  contents.phase = static_cast<int>(phase_);
  contents.last_player_to_go = last_player_to_go_;
  contents.shuffle_pending = shuffle_pending_;
  contents.shuffle_pending_end_of_turn = shuffle_pending_end_of_turn_;
  contents.original_player_for_shuffle = original_player_for_shuffle_;
  contents.pending_draw_count_after_shuffle = pending_draw_count_after_shuffle_;
  contents.supply_piles = supply_piles_;
  contents.initial_supply_piles = initial_supply_piles_;
  contents.play_area.clear();
  contents.play_area.reserve(play_area_.size());
  for (auto cn : play_area_) contents.play_area.push_back(static_cast<int>(cn));
  contents.player_states.clear();
  contents.player_states.reserve(kNumPlayers);
  for (int p = 0; p < kNumPlayers; ++p) {
    auto pstruct = player_states_[p].ToStruct();
    nlohmann::json pj = pstruct->to_json_base();
    contents.player_states.push_back(pj.get<DominionPlayerStructContents>());
  }
  contents.move_number = move_number_;
  auto ss = std::make_unique<DominionStateStruct>();
  static_cast<DominionStateStructContents &>(*ss) = contents;
  return ss;
}

std::string DominionState::Serialize() const { return ToJson(); }

// Per-player observation string: only include public info and the player's own
// privates.
// TODO: Add information about deck and discard tracking, opponent deck
// tracking, etc.
std::string DominionState::ObservationString(int player) const {
  const auto &ps_me = player_states_[player];
  const auto &ps_opp = player_states_[1 - player];
  auto card_name = [](CardName cn) { return GetCardSpec(cn).name_; };

  std::string s;
  s += std::string("Player: ") + std::to_string(player) + "\n";
  s += std::string("Phase: ") +
       (phase_ == Phase::actionPhase ? "Action" : "Buy") + "\n";
  s += std::string("Actions: ") + std::to_string(actions_) + "\n";
  s += std::string("Buys: ") + std::to_string(buys_) + "\n";
  s += std::string("Coins: ") + std::to_string(coins_) + "\n";

  // Show only this player's hand contents; deck and discard sizes only.
  s += "Hand: ";
  bool first_hand = true;
  for (int j = 0; j < kNumSupplyPiles; ++j) {
    int cnt = ps_me.hand_counts_[j];
    if (cnt <= 0) continue;
    if (!first_hand) s += " ";
    first_hand = false;
    s += card_name(static_cast<CardName>(j)) + std::string("x") + std::to_string(cnt);
  }
  s += "\n";
  s += std::string("DeckSize: ") + std::to_string(ps_me.deck_.size()) + "\n";
  int discard_me_sz = 0; for (int j=0;j<kNumSupplyPiles;++j) discard_me_sz += ps_me.discard_counts_[j];
  s += std::string("DiscardSize: ") + std::to_string(discard_me_sz) +
       "\n";

  // Opponent privates are hidden; expose sizes only.
  int opp_hand_size = 0; for (int j=0;j<kNumSupplyPiles;++j) opp_hand_size += ps_opp.hand_counts_[j];
  s += std::string("OpponentHandSize: ") + std::to_string(opp_hand_size) +
       "\n";
  s += std::string("OpponentDeckSize: ") + std::to_string(ps_opp.deck_.size()) +
       "\n";
  int discard_opp_sz = 0; for (int j=0;j<kNumSupplyPiles;++j) discard_opp_sz += ps_opp.discard_counts_[j];
  s += std::string("OpponentDiscardSize: ") +
       std::to_string(discard_opp_sz) + "\n";

  // Public supply counts.
  s += "Supply: ";
  bool first_supply = true;
  for (int i = 0; i < kNumSupplyPiles; ++i) {
    if (initial_supply_piles_[i] == 0) continue;
    if (!first_supply) s += ", ";
    first_supply = false;
    s += std::to_string(i);
    s += ":";
    s += card_name(static_cast<CardName>(i));
    s += "=" + std::to_string(supply_piles_[i]);
  }
  s += "\n";

  // Public play area.
  s += "PlayArea: ";
  for (size_t i = 0; i < play_area_.size(); ++i) {
    if (i)
      s += " ";
    s += card_name(play_area_[i]);
  }
  s += "\n";

  // Append last public action and current legal actions (only for current
  // player).
  const auto h = History();
  if (!h.empty()) {
    s += "LastAction: ";
    s += FormatActionPair(*this, h.back());
    s += "\n";
  }
  if (player == current_player_) {
    s += "LegalActions: ";
    const auto las = LegalActions();
    for (size_t i = 0; i < las.size(); ++i) {
      if (i)
        s += ", ";
      const Action a = las[i];
      std::string a_str = FormatActionPair(*this, a);
      s += a_str;
    }
  }
  return s;
}

// Information state string: perfect recall view for the player.
// Include public info and the player's private info plus full public history.
std::string DominionState::InformationStateString(int player) const {
  std::string s = ObservationString(player);
  // Include last action and legal actions for current player (already safe to
  // expose).
  const auto h = History();
  if (!h.empty()) {
    s += "\nLastAction: ";
    s += FormatActionPair(*this, h.back());
  }
  return s;
}

std::string DominionState::ToString() const {
  return std::string("DominionState_Turn_") + std::to_string(turn_number_) +
         std::string("_Player_") + std::to_string(current_player_);
}

bool DominionState::IsTerminal() const {
  if (supply_piles_[static_cast<int>(CardName::CARD_Province)] == 0) return true;
  int empty = 0;
  for (int i = 0; i < kNumSupplyPiles; ++i) {
    if (initial_supply_piles_[i] > 0 && supply_piles_[i] == 0) empty += 1;
  }
  return empty >= 3;
}

static int CountVP(const PlayerState &ps) {
  int vp = 0;
  auto count_all = [&](CardName name) {
    int idx = static_cast<int>(name);
    int in_hand = (idx >= 0 && idx < kNumSupplyPiles) ? ps.hand_counts_[idx] : 0;
    int in_discard = (idx >= 0 && idx < kNumSupplyPiles) ? ps.discard_counts_[idx] : 0;
    return std::count(ps.deck_.begin(), ps.deck_.end(), name) + in_discard + in_hand;
  };
  int estates = count_all(CardName::CARD_Estate);
  int duchies = count_all(CardName::CARD_Duchy);
  int provinces = count_all(CardName::CARD_Province);
  int curses = count_all(CardName::CARD_Curse);
  int gardens = count_all(CardName::CARD_Gardens);
  int total_cards = static_cast<int>(ps.deck_.size());
  for (int j = 0; j < kNumSupplyPiles; ++j) total_cards += ps.discard_counts_[j];
  for (int j = 0; j < kNumSupplyPiles; ++j) total_cards += ps.hand_counts_[j];
  vp += estates * 1 + duchies * 3 + provinces * 6;
  vp -= curses * 1;
  vp += gardens * (total_cards / 10);
  return vp;
}

std::vector<double> DominionState::Returns() const {
  if (!IsTerminal())
    return std::vector<double>(kNumPlayers, 0.0);
  int vp0 = CountVP(player_states_[0]);
  int vp1 = CountVP(player_states_[1]);
  if (vp0 > vp1)
    return {1.0, -1.0};
  if (vp1 > vp0)
    return {-1.0, 1.0};
  if (last_player_to_go_ == 1)
    return {0.0, 0.0};
  return {-1.0, 1.0};
}

std::unique_ptr<State> DominionState::Clone() const {
  return std::unique_ptr<State>(new DominionState(*this));
}

// Applies the given action_id for the current player.
// - Delegates effect-specific resolution first (e.g., discard selection).
// - Handles phase transitions: EndActions -> buyPhase; EndBuy -> cleanup + next
// turn.
void DominionState::DoApplyAction(Action action_id) {
  if (IsChanceNode()) {
    SPIEL_CHECK_TRUE(shuffle_pending_);
    SPIEL_CHECK_EQ(action_id, ActionIds::Shuffle());
    // Localize RNG for shuffling discard into deck; reproducibility is not required.
    unsigned seed = static_cast<unsigned>(std::random_device{}());
    std::mt19937 local_rng(seed);
    auto &ps_orig = player_states_[original_player_for_shuffle_];
    int discard_size = 0; for (int jj=0;jj<kNumSupplyPiles;++jj) discard_size += ps_orig.discard_counts_[jj];
    if (discard_size > 0) {
      std::vector<CardName> tmp;
      tmp.reserve(discard_size);
      for (int jj = 0; jj < kNumSupplyPiles; ++jj) {
        for (int k = 0; k < ps_orig.discard_counts_[jj]; ++k) tmp.push_back(static_cast<CardName>(jj));
        ps_orig.discard_counts_[jj] = 0;
      }
      std::shuffle(tmp.begin(), tmp.end(), local_rng);
      ps_orig.deck_.insert(ps_orig.deck_.end(), tmp.begin(), tmp.end());
    }
    shuffle_pending_ = false;
    Player resume_player = original_player_for_shuffle_;
    original_player_for_shuffle_ = -1;
    int to_draw = pending_draw_count_after_shuffle_;
    pending_draw_count_after_shuffle_ = 0;
    DrawCardsFor(resume_player, to_draw);
    if (shuffle_pending_end_of_turn_) {
      shuffle_pending_end_of_turn_ = false;
      current_player_ = 1 - resume_player;
      phase_ = Phase::actionPhase;
      MaybeAutoAdvanceToBuyPhase();
    }
    return;
  }
  auto &ps = player_states_[current_player_];
  // If there is a pending effect at the front of the queue and it provides an
  // action handler, delegate to it first.
  if (!ps.effect_queue.empty() && ps.pending_choice != PendingChoice::None &&
      ps.effect_queue.front() && ps.effect_queue.front()->on_action) {
    bool consumed =
        ps.effect_queue.front()->on_action(*this, current_player_, action_id);
    if (consumed) {
      MaybeAutoAdvanceToBuyPhase();
      MaybeAutoApplySingleAction();
      return;
    }
  }
  if (phase_ == Phase::actionPhase) {
    if (action_id == ActionIds::EndActions()) {
      phase_ = Phase::buyPhase;
      MaybeAutoApplySingleAction();
      return;
    }
    if (action_id < ActionIds::MaxHandSize() && actions_ > 0) {
      int j = static_cast<int>(action_id);
      SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
      SPIEL_CHECK_TRUE(ps.hand_counts_[j] > 0);
      CardName cn = static_cast<CardName>(j);
      const Card &spec = GetCardSpec(cn);
      if (HasType(spec, CardType::ACTION)) {
        play_area_.push_back(cn);
        ps.hand_counts_[j] -= 1;
        actions_ -= 1;
        spec.Play(*this, current_player_);
      }
    }
  } else if (phase_ == Phase::buyPhase) {
    if (action_id == ActionIds::EndBuy()) {
      EndBuyCleanup();
      return;
    }
    if (action_id < ActionIds::MaxHandSize()) {
      int j = static_cast<int>(action_id);
      SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
      SPIEL_CHECK_TRUE(ps.hand_counts_[j] > 0);
      CardName cn = static_cast<CardName>(j);
      const Card &spec = GetCardSpec(cn);
      if (HasType(spec, CardType::BASIC_TREASURE) || HasType(spec, CardType::SPECIAL_TREASURE)) {
        play_area_.push_back(cn);
        ps.hand_counts_[j] -= 1;
        spec.applyGrants(*this, current_player_);
      }
      MaybeAutoApplySingleAction();
      return;
    }
    if (action_id >= ActionIds::BuyBase() &&
        action_id < ActionIds::BuyBase() + kNumSupplyPiles && buys_ > 0) {
      int j = action_id - ActionIds::BuyBase();
      SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
      // Auto-play all basic treasures in hand before attempting the purchase.
      for (int t = 0; t < kNumSupplyPiles; ++t) {
        if (ps.hand_counts_[t] <= 0) continue;
        const Card &tspec = GetCardSpec(static_cast<CardName>(t));
        if (!HasType(tspec, CardType::BASIC_TREASURE)) continue;
        int c = ps.hand_counts_[t];
        for (int k = 0; k < c; ++k) {
          ApplyAction(ActionIds::PlayHandIndex(t));
        }
      }
      if (supply_piles_[j] > 0) {
        const Card &spec = GetCardSpec(static_cast<CardName>(j));
        if (coins_ >= spec.cost_) {
          coins_ -= spec.cost_;
          buys_ -= 1;
          ps.discard_counts_[j] += 1;
          supply_piles_[j] -= 1;
          if (buys_ == 0) {
            EndBuyCleanup();
            return;
          }
        }
      }
      MaybeAutoApplySingleAction();
      return;
    }
  }
}

void DominionState::EndBuyCleanup() {
  // Cleanup end of turn for current_player_
  auto &ps = player_states_[current_player_];
  last_player_to_go_ = current_player_;
  // Move all hand counts to discard.
  for (int j = 0; j < kNumSupplyPiles; ++j) {
    int cnt = ps.hand_counts_[j];
    ps.discard_counts_[j] += cnt;
    ps.hand_counts_[j] = 0;
  }
  for (auto c : play_area_) {
    int idx = static_cast<int>(c);
    if (idx >= 0 && idx < kNumSupplyPiles) ps.discard_counts_[idx] += 1;
  }
  play_area_.clear();

  // Reset and switch the next player
  coins_ = 0;
  actions_ = 1;
  buys_ = 1;
  turn_number_ += 1;
  DrawCardsFor(current_player_, 5);
  current_player_ = 1 - current_player_;
  phase_ = Phase::actionPhase;
  // Optimization: if the next player has no legal action plays or zero actions,
  // move directly to buy phase to skip an unnecessary EndActions.
  MaybeAutoAdvanceToBuyPhase();
}

void DominionState::MaybeAutoAdvanceToBuyPhase() {
  // Only consider auto-advancing when in action phase and not in the middle of
  // resolving an effect/choice.
  auto &ps = player_states_[current_player_];
  if (phase_ != Phase::actionPhase) return;
  if (!ps.effect_queue.empty() || ps.pending_choice != PendingChoice::None) return;

  // If the player has zero actions, or has no action cards in hand, switch.
  if (actions_ <= 0) {
    phase_ = Phase::buyPhase;
    return;
  }

  bool has_playable_action = false;
  for (int j = 0; j < kNumSupplyPiles; ++j) {
    if (ps.hand_counts_[j] <= 0) continue;
    const Card &spec = GetCardSpec(static_cast<CardName>(j));
    if (HasType(spec, CardType::ACTION)) { has_playable_action = true; break; }
  }
  if (!has_playable_action) phase_ = Phase::buyPhase;
  // After phase adjustments, opportunistically auto-apply when only one legal
  // action remains (excluding chance/shuffle).
  MaybeAutoApplySingleAction();
}

void DominionState::MaybeAutoApplySingleAction() {
  // Do not auto-apply during chance nodes; respect shuffle prompts.
  if (IsTerminal() || IsChanceNode()) return;
  // Limit iteration to prevent pathological loops.
  int guard = 0;
  while (!IsTerminal() && !IsChanceNode()) {
    auto las = LegalActions();
    if (las.size() != 1) break;
    ApplyAction(las[0]);
    guard += 1;
    if (guard > kDominionMaxDistinctActions) break;
  }
}

ActionsAndProbs DominionState::ChanceOutcomes() const {
  return ActionsAndProbs{{ActionIds::Shuffle(), 1.0}};
}

} // namespace dominion
} // namespace open_spiel
