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
namespace {

const GameType kGameType{
    "dominion",
    "Dominion (Base)",
    GameType::Dynamics::kSequential,
    GameType::ChanceMode::kDeterministic,
    GameType::Information::kImperfectInformation,
    GameType::Utility::kZeroSum,
    GameType::RewardModel::kTerminal,
    /*max_num_players=*/kNumPlayers,
    /*min_num_players=*/kNumPlayers,
    /*provides_information_state_string=*/true,
    /*provides_information_state_tensor=*/false,
    /*provides_observation_string=*/true,
    /*provides_observation_tensor=*/false,
    /*parameter_specification=*/{{"rng_seed", GameParameter(0)}}};

std::shared_ptr<const Game> Factory(const GameParameters &params) {
  return std::shared_ptr<const Game>(new DominionGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

} // namespace

DominionGame::DominionGame(const GameParameters &params)
    : Game(kGameType, params) {
  // Seed the game's RNG from the parameter for reproducibility across runs.
  rng_seed_ = ParameterValue<int>("rng_seed");
  rng_.seed(static_cast<unsigned>(rng_seed_));
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

std::string DominionGame::GetRNGState() const {
  // Serialize the RNG so Game::Serialize captures deterministic randomness.
  std::ostringstream oss;
  oss << rng_;
  return oss.str();
}

void DominionGame::SetRNGState(const std::string &rng_state) const {
  // Restore RNG from a serialized string to reproduce stochastic outcomes.
  if (rng_state.empty())
    return;
  std::istringstream iss(rng_state);
  iss >> rng_;
}

namespace {
static bool HasType(const Card &c, CardType t) {
  return std::find(c.types_.begin(), c.types_.end(), t) != c.types_.end();
}

// Format an action as "id:name" using the game's action-naming helpers.
static std::string FormatActionPair(Action a) {
  return std::to_string(static_cast<int>(a)) + ":" +
         ActionNames::Name(a, kNumSupplyPiles);
}
} // namespace

void DominionState::DrawCardsFor(int player, int n) {
  auto &ps = player_states_[player];
  // Shuffle discard into deck when deck is empty; use game RNG for determinism.
  const auto *dom_game = dynamic_cast<const DominionGame *>(game_.get());
  std::mt19937 *rng = dom_game ? dom_game->rng() : nullptr;
  auto shuffle_vec = [&](std::vector<CardName> &v) {
    if (rng)
      std::shuffle(v.begin(), v.end(), *rng);
  };
  for (int i = 0; i < n; ++i) {
    if (ps.deck_.empty()) {
      if (ps.discard_.empty())
        break;
      shuffle_vec(ps.discard_);
      ps.deck_.insert(ps.deck_.end(), ps.discard_.begin(), ps.discard_.end());
      ps.discard_.clear();
    }
    int idx = static_cast<int>(ps.deck_.back());
    if (idx >= 0 && idx < kNumSupplyPiles) {
      ps.hand_counts_[idx] += 1;
    }
    ps.deck_.pop_back();
  }
}

DominionState::DominionState(std::shared_ptr<const Game> game) : State(game) {
  // Shuffle initial decks using game RNG for reproducibility.
  const auto *dom_game = dynamic_cast<const DominionGame *>(game_.get());
  std::mt19937 *rng = dom_game ? dom_game->rng() : nullptr;

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
                                      CardName::CARD_Mine,    CardName::CARD_Moat,    CardName::CARD_Remodel,
                                      CardName::CARD_Smithy,  CardName::CARD_Village, CardName::CARD_Workshop,
                                      CardName::CARD_Festival};
  for (CardName cn : kingdom) supply_piles_[static_cast<int>(cn)] = 10;
  initial_supply_piles_ = supply_piles_;

  // Initial decks and hands
  for (int p = 0; p < kNumPlayers; ++p) {
    auto &ps = player_states_[p];
    ps.deck_.clear();
    ps.discard_.clear();
    ps.hand_counts_.fill(0);
    ps.obs_state =
        std::make_unique<ObservationState>(ps.hand_counts_, ps.deck_, ps.discard_);
    for (int i = 0; i < 7; ++i) {
      ps.deck_.push_back(CardName::CARD_Copper);
    }
    for (int i = 0; i < 3; ++i) {
      ps.deck_.push_back(CardName::CARD_Estate);
    }
    if (rng) {
      std::shuffle(ps.deck_.begin(), ps.deck_.end(), *rng);
    }

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

Player DominionState::CurrentPlayer() const { return current_player_; }

// Computes the legal actions for the current player.
// Returns sorted IDs and delegates to pending-effect logic first.
std::vector<Action> DominionState::LegalActions() const {
  std::vector<Action> actions;
  if (IsTerminal())
    return actions;
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
    for (int j = 0; j < kNumSupplyPiles; ++j) {
      if (ps.hand_counts_[j] <= 0) continue;
      const Card &spec = GetCardSpec(static_cast<CardName>(j));
      if (HasType(spec, CardType::BASIC_TREASURE) || HasType(spec, CardType::SPECIAL_TREASURE)) {
        actions.push_back(ActionIds::PlayHandIndex(j));
      }
    }
    if (buys_ > 0) {
      for (int j = 0; j < kNumSupplyPiles; ++j) {
        if (supply_piles_[j] <= 0) continue;
        const Card &spec = GetCardSpec(static_cast<CardName>(j));
        if (coins_ >= spec.cost_) actions.push_back(ActionIds::BuyFromSupply(j));
      }
    }
    actions.push_back(ActionIds::EndBuy());
  }
  std::sort(actions.begin(), actions.end());
  return actions;
}

std::string DominionState::ActionToString(Player /*player*/,
                                          Action action_id) const {
  return ActionNames::NameWithCard(action_id, kNumSupplyPiles);
}

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
  s += std::string("DiscardSize: ") + std::to_string(ps_me.discard_.size()) +
       "\n";

  // Opponent privates are hidden; expose sizes only.
  int opp_hand_size = 0; for (int j=0;j<kNumSupplyPiles;++j) opp_hand_size += ps_opp.hand_counts_[j];
  s += std::string("OpponentHandSize: ") + std::to_string(opp_hand_size) +
       "\n";
  s += std::string("OpponentDeckSize: ") + std::to_string(ps_opp.deck_.size()) +
       "\n";
  s += std::string("OpponentDiscardSize: ") +
       std::to_string(ps_opp.discard_.size()) + "\n";

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
    s += FormatActionPair(h.back());
    s += "\n";
  }
  if (player == current_player_) {
    s += "LegalActions: ";
    const auto las = LegalActions();
    for (size_t i = 0; i < las.size(); ++i) {
      if (i)
        s += ", ";
      const Action a = las[i];
      std::string a_str = FormatActionPair(a);
      if (a < ActionIds::MaxHandSize()) {
        int idx = static_cast<int>(a);
        if (idx >= 0 && idx < kNumSupplyPiles) {
          a_str += " (" + card_name(static_cast<CardName>(idx)) + ")";
        }
      } else if (a >= ActionIds::BuyBase() && a < ActionIds::BuyBase() + kNumSupplyPiles) {
        int j = static_cast<int>(a) - ActionIds::BuyBase();
        if (j >= 0 && j < kNumSupplyPiles) {
          a_str += " (" + card_name(static_cast<CardName>(j)) + ")";
        }
      }
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
    s += FormatActionPair(h.back());
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
    return std::count(ps.deck_.begin(), ps.deck_.end(), name) +
           std::count(ps.discard_.begin(), ps.discard_.end(), name) +
           in_hand;
  };
  int estates = count_all(CardName::CARD_Estate);
  int duchies = count_all(CardName::CARD_Duchy);
  int provinces = count_all(CardName::CARD_Province);
  int curses = count_all(CardName::CARD_Curse);
  int gardens = count_all(CardName::CARD_Gardens);
  int total_cards = static_cast<int>(ps.deck_.size() + ps.discard_.size());
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
  auto &ps = player_states_[current_player_];
  // If there is a pending effect node and it provides an action handler,
  // delegate to it first.
  if (ps.effect_head && ps.pending_choice != PendingChoice::None &&
      ps.effect_head->on_action) {
    bool consumed =
        ps.effect_head->on_action(*this, current_player_, action_id);
    if (consumed) {
      if (ps.pending_choice == PendingChoice::None && ps.effect_head) {
        ps.effect_head = std::move(ps.effect_head->next);
      }

      // After advancing the effect chain, process any pending Throne Room
      // replays.
      while (ps.pending_choice == PendingChoice::None && !ps.effect_head &&
             !ps.pending_throne_replay_stack.empty()) {
        CardName to_replay = ps.pending_throne_replay_stack.back();
        ps.pending_throne_replay_stack.pop_back();
        const Card &spec = GetCardSpec(to_replay);
        // Throne Room replay: apply standard grants and the card effect,
        // without removing from hand, moving to play area, or consuming an
        // action.
        spec.play(*this, current_player_);
        spec.applyEffect(*this, current_player_);
        // If this creates a pending effect (e.g., Throne Room selection), break
        // to let it resolve.
        if (ps.effect_head) {
          // If we just replayed a Throne card and need to schedule its second
          // play, do so now.
          if (to_replay == CardName::CARD_ThroneRoom &&
              ps.pending_throne_schedule_second_for.has_value() &&
              ps.pending_throne_schedule_second_for.value() == to_replay) {
            ps.pending_throne_replay_stack.push_back(to_replay);
            ps.pending_throne_schedule_second_for.reset();
          }
          break;
        }
      }
      // If we are in action phase and there is no longer any pending effect,
      // auto-advance to buy phase when appropriate.
      MaybeAutoAdvanceToBuyPhase();
      return;
    }
  }
  if (phase_ == Phase::actionPhase) {
    if (action_id == ActionIds::EndActions()) {
      phase_ = Phase::buyPhase;
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
        spec.play(*this, current_player_);
        spec.applyEffect(*this, current_player_);
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
        spec.play(*this, current_player_);
      }
      return;
    }
    if (action_id >= ActionIds::BuyBase() &&
        action_id < ActionIds::BuyBase() + kNumSupplyPiles && buys_ > 0) {
      int j = action_id - ActionIds::BuyBase();
      SPIEL_CHECK_TRUE(j >= 0 && j < kNumSupplyPiles);
      if (supply_piles_[j] > 0) {
        const Card &spec = GetCardSpec(static_cast<CardName>(j));
        if (coins_ >= spec.cost_) {
          coins_ -= spec.cost_;
          buys_ -= 1;
          ps.discard_.push_back(static_cast<CardName>(j));
          supply_piles_[j] -= 1;
          if (buys_ == 0) {
            EndBuyCleanup();
            return;
          }
        }
      }
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
    for (int k = 0; k < cnt; ++k) ps.discard_.push_back(static_cast<CardName>(j));
    ps.hand_counts_[j] = 0;
  }
  auto move_all_vec = [&](std::vector<CardName> &from) {
    for (auto c : from) ps.discard_.push_back(c);
    from.clear();
  };
  move_all_vec(play_area_);
  move_all_vec(play_area_);

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
  if (ps.effect_head || ps.pending_choice != PendingChoice::None) return;

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
}

} // namespace dominion
} // namespace open_spiel
