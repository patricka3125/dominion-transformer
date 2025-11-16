#include <cstdlib>
#include <algorithm>
#include <random>
#include <map>

#include "dominion.hpp"
#include "cards.hpp"
#include "actions.hpp"
#include "open_spiel/spiel.h"

namespace open_spiel {
namespace dominion {
namespace {

const GameType kGameType{
    "dominion",
    "Dominion (Base)",
    GameType::Dynamics::kSequential,
    GameType::ChanceMode::kSampledStochastic,
    GameType::Information::kImperfectInformation,
    GameType::Utility::kGeneralSum,
    GameType::RewardModel::kTerminal,
    /*max_num_players=*/kNumPlayers,
    /*min_num_players=*/kNumPlayers,
    /*provides_information_state_string=*/true,
    /*provides_information_state_tensor=*/false,
    /*provides_observation_string=*/true,
    /*provides_observation_tensor=*/false,
    /*parameter_specification=*/{{"rng_seed", GameParameter(0)}}
};

std::shared_ptr<const Game> Factory(const GameParameters& params) {
  return std::shared_ptr<const Game>(new DominionGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

}  // namespace

DominionGame::DominionGame(const GameParameters& params)
    : Game(kGameType, params) {
  // Seed the game's RNG from the parameter for reproducibility across runs.
  rng_seed_ = ParameterValue<int>("rng_seed");
  rng_.seed(static_cast<unsigned>(rng_seed_));
}

int DominionGame::NumDistinctActions() const { return kDominionMaxDistinctActions; }

std::unique_ptr<State> DominionGame::NewInitialState() const {
  return std::unique_ptr<State>(new DominionState(shared_from_this()));
}

int DominionGame::NumPlayers() const { return kNumPlayers; }

double DominionGame::MinUtility() const { return -50.0; }

double DominionGame::MaxUtility() const { return 50.0; }

std::vector<int> DominionGame::InformationStateTensorShape() const { return {}; }

std::vector<int> DominionGame::ObservationTensorShape() const { return {}; }

int DominionGame::MaxGameLength() const { return 500; }

std::string DominionGame::GetRNGState() const {
  // Serialize the RNG so Game::Serialize captures deterministic randomness.
  std::ostringstream oss;
  oss << rng_;
  return oss.str();
}

void DominionGame::SetRNGState(const std::string& rng_state) const {
  // Restore RNG from a serialized string to reproduce stochastic outcomes.
  if (rng_state.empty()) return;
  std::istringstream iss(rng_state);
  iss >> rng_;
}

namespace {
static bool HasType(const Card& c, CardType t) {
  return std::find(c.types_.begin(), c.types_.end(), t) != c.types_.end();
}
}

void DominionState::DrawCardsFor(int player, int n) {
  auto& ps = player_states_[player];
  // Shuffle discard into deck when deck is empty; use game RNG for determinism.
  const auto* dom_game = dynamic_cast<const DominionGame*>(game_.get());
  std::mt19937* rng = dom_game ? dom_game->rng() : nullptr;
  auto shuffle_vec = [&](std::vector<CardName>& v) {
    if (rng) std::shuffle(v.begin(), v.end(), *rng);
  };
  for (int i = 0; i < n; ++i) {
    if (ps.deck_.empty()) {
      if (ps.discard_.empty()) break;
      shuffle_vec(ps.discard_);
      ps.deck_.insert(ps.deck_.end(), ps.discard_.begin(), ps.discard_.end());
      ps.discard_.clear();
    }
    ps.hand_.push_back(ps.deck_.back());
    ps.deck_.pop_back();
  }
}

DominionState::DominionState(std::shared_ptr<const Game> game)
    : State(game) {
  // Shuffle initial decks using game RNG for reproducibility.
  const auto* dom_game = dynamic_cast<const DominionGame*>(game_.get());
  std::mt19937* rng = dom_game ? dom_game->rng() : nullptr;
  // Supply types: base and kingdom piles
  supply_types_[0] = CardName::CARD_Copper;
  supply_types_[1] = CardName::CARD_Silver;
  supply_types_[2] = CardName::CARD_Gold;
  supply_types_[3] = CardName::CARD_Estate;
  supply_types_[4] = CardName::CARD_Duchy;
  supply_types_[5] = CardName::CARD_Province;
  supply_types_[6] = CardName::CARD_Curse;
  supply_types_[7] = CardName::CARD_Cellar;
  supply_types_[8] = CardName::CARD_Market;
  supply_types_[9] = CardName::CARD_Militia;
  supply_types_[10] = CardName::CARD_Mine;
  supply_types_[11] = CardName::CARD_Moat;
  supply_types_[12] = CardName::CARD_Remodel;
  supply_types_[13] = CardName::CARD_Smithy;
  supply_types_[14] = CardName::CARD_Village;
  supply_types_[15] = CardName::CARD_Workshop;
  supply_types_[16] = CardName::CARD_Festival;

  // Supply counts
  supply_piles_[0] = 60 - 7 * kNumPlayers;
  supply_piles_[1] = 40;
  supply_piles_[2] = 30;
  supply_piles_[3] = 8;
  supply_piles_[4] = 8;
  supply_piles_[5] = 8;
  supply_piles_[6] = 10;
  for (int i = 7; i < kNumSupplyPiles; ++i) supply_piles_[i] = 10;

  // Initial decks and hands
  for (int p = 0; p < kNumPlayers; ++p) {
    auto& ps = player_states_[p];
    ps.deck_.clear();
    ps.discard_.clear();
    ps.hand_.clear();
    for (int i = 0; i < 7; ++i) ps.deck_.push_back(CardName::CARD_Copper);
    for (int i = 0; i < 3; ++i) ps.deck_.push_back(CardName::CARD_Estate);
    if (rng) {
      std::shuffle(ps.deck_.begin(), ps.deck_.end(), *rng);
    }
    DrawCardsFor(p, 5);
  }

  supply_piles_[0] -= 7 * kNumPlayers;
  supply_piles_[3] -= 3 * kNumPlayers;

  current_player_ = 0;
  actions_ = 1;
  buys_1 = 1;
  coins_ = 0;
  phase_ = Phase::actionPhase;
}

Player DominionState::CurrentPlayer() const { return current_player_; }

// Computes the legal actions for the current player.
// Returns sorted IDs and delegates to pending-effect logic first.
std::vector<Action> DominionState::LegalActions() const {
  std::vector<Action> actions;
  if (IsTerminal()) return actions;
  const auto& ps = player_states_[current_player_];
  {
    auto pend = PendingEffectLegalActions(*this, current_player_);
    if (!pend.empty()) return pend;
  }
  if (phase_ == Phase::actionPhase) {
    if (actions_ > 0) {
      for (int i = 0; i < static_cast<int>(ps.hand_.size()) && i < 100; ++i) {
        const Card& spec = GetCardSpec(ps.hand_[i]);
        if (HasType(spec, CardType::ACTION)) actions.push_back(ActionIds::PlayHandIndex(i));
      }
    }
    actions.push_back(ActionIds::EndActions());
  } else if (phase_ == Phase::buyPhase) {
    for (int i = 0; i < static_cast<int>(ps.hand_.size()) && i < 100; ++i) {
      const Card& spec = GetCardSpec(ps.hand_[i]);
      if (HasType(spec, CardType::TREASURE)) actions.push_back(ActionIds::PlayHandIndex(i));
    }
    if (buys_1 > 0) {
      for (int j = 0; j < kNumSupplyPiles; ++j) {
        if (supply_piles_[j] <= 0) continue;
        const Card& spec = GetCardSpec(supply_types_[j]);
        if (coins_ >= spec.cost_) actions.push_back(ActionIds::BuyFromSupply(j));
      }
    }
    actions.push_back(ActionIds::EndBuy());
  }
  std::sort(actions.begin(), actions.end());
  return actions;
}

std::string DominionState::ActionToString(Player /*player*/, Action action_id) const {
  return ActionNames::Name(action_id, kNumSupplyPiles);
}

// Per-player observation string: only include public info and the player's own privates.
std::string DominionState::ObservationString(int player) const {
  const auto& ps_me = player_states_[player];
  const auto& ps_opp = player_states_[1 - player];
  auto card_name = [](CardName cn) { return GetCardSpec(cn).name_; };

  std::string s;
  s += std::string("Player: ") + std::to_string(player) + "\n";
  s += std::string("Phase: ") + (phase_ == Phase::actionPhase ? "Action" : "Buy") + "\n";
  s += std::string("Actions: ") + std::to_string(actions_) + "\n";
  s += std::string("Buys: ") + std::to_string(buys_1) + "\n";
  s += std::string("Coins: ") + std::to_string(coins_) + "\n";

  // Show only this player's hand contents; deck and discard sizes only.
  s += "Hand: ";
  for (size_t i = 0; i < ps_me.hand_.size(); ++i) {
    if (i) s += " ";
    s += card_name(ps_me.hand_[i]);
  }
  s += "\n";
  s += std::string("DeckSize: ") + std::to_string(ps_me.deck_.size()) + "\n";
  s += std::string("DiscardSize: ") + std::to_string(ps_me.discard_.size()) + "\n";

  // Opponent privates are hidden; expose sizes only.
  s += std::string("OpponentHandSize: ") + std::to_string(ps_opp.hand_.size()) + "\n";
  s += std::string("OpponentDeckSize: ") + std::to_string(ps_opp.deck_.size()) + "\n";
  s += std::string("OpponentDiscardSize: ") + std::to_string(ps_opp.discard_.size()) + "\n";

  // Public supply counts.
  s += "SupplyCounts: ";
  for (int i = 0; i < kNumSupplyPiles; ++i) {
    if (i) s += ",";
    s += std::to_string(supply_piles_[i]);
  }
  s += "\n";

  // Public play area.
  s += "PlayArea: ";
  for (size_t i = 0; i < play_area_.size(); ++i) {
    if (i) s += " ";
    s += card_name(play_area_[i]);
  }
  return s;
}

// Information state string: perfect recall view for the player.
// Include public info and the player's private info plus full public history.
std::string DominionState::InformationStateString(int player) const {
  std::string s = ObservationString(player);
  s += "History: ";
  const auto h = History();
  for (size_t i = 0; i < h.size(); ++i) {
    if (i) s += ",";
    s += std::to_string(static_cast<int>(h[i]));
  }
  return s;
}

std::string DominionState::ToString() const {
  return std::string("DominionState_Turn_") + std::to_string(turn_number_) +
         std::string("_Player_") + std::to_string(current_player_);
}

bool DominionState::IsTerminal() const {
  int empty = 0;
  for (int i = 0; i < kNumSupplyPiles; ++i) empty += (supply_piles_[i] == 0);
  if (supply_piles_[5] == 0) return true;
  if (empty >= 3) return true;
  return false;
}

static int CountVP(const PlayerState& ps) {
  int vp = 0;
  auto count_all = [&](CardName name) {
    return std::count(ps.deck_.begin(), ps.deck_.end(), name) +
           std::count(ps.discard_.begin(), ps.discard_.end(), name) +
           std::count(ps.hand_.begin(), ps.hand_.end(), name);
  };
  int estates = count_all(CardName::CARD_Estate);
  int duchies = count_all(CardName::CARD_Duchy);
  int provinces = count_all(CardName::CARD_Province);
  int curses = count_all(CardName::CARD_Curse);
  int gardens = count_all(CardName::CARD_Gardens);
  int total_cards = static_cast<int>(ps.deck_.size() + ps.discard_.size() + ps.hand_.size());
  vp += estates * 1 + duchies * 3 + provinces * 6;
  vp -= curses * 1;
  vp += gardens * (total_cards / 10);
  return vp;
}

std::vector<double> DominionState::Returns() const {
  if (!IsTerminal()) return std::vector<double>(kNumPlayers, 0.0);
  int vp0 = CountVP(player_states_[0]);
  int vp1 = CountVP(player_states_[1]);
  if (vp0 > vp1) return {1.0, -1.0};
  if (vp1 > vp0) return {-1.0, 1.0};
  if (last_player_to_go_ == 1) return {0.0, 0.0};
  return {-1.0, 1.0};
}

std::unique_ptr<State> DominionState::Clone() const {
  return std::unique_ptr<State>(new DominionState(*this));
}

// Applies the given action_id for the current player.
// - Delegates effect-specific resolution first (e.g., discard selection).
// - Handles phase transitions: EndActions -> buyPhase; EndBuy -> cleanup + next turn.
void DominionState::DoApplyAction(Action action_id) {
  auto& ps = player_states_[current_player_];
  // If there is a pending effect node and it provides an action handler,
  // delegate to it first.
  if (ps.effect_head && ps.pending_choice != PendingChoice::None && ps.effect_head->on_action) {
    bool consumed = ps.effect_head->on_action(*this, current_player_, action_id);
    if (consumed) {
      if (ps.pending_choice == PendingChoice::None && ps.effect_head) {
        ps.effect_head = std::move(ps.effect_head->next);
      }

      // After advancing the effect chain, process any pending Throne Room replays.
      while (ps.pending_choice == PendingChoice::None && !ps.effect_head && !ps.pending_throne_replay_stack.empty()) {
        CardName to_replay = ps.pending_throne_replay_stack.back();
        ps.pending_throne_replay_stack.pop_back();
        const Card& spec = GetCardSpec(to_replay);
        // Throne Room replay: apply standard grants and the card effect,
        // without removing from hand, moving to play area, or consuming an action.
        spec.play(*this, current_player_);
        spec.applyEffect(*this, current_player_);
        // If this creates a pending effect (e.g., Throne Room selection), break to let it resolve.
        if (ps.effect_head) {
          // If we just replayed a Throne card and need to schedule its second play, do so now.
          if (to_replay == CardName::CARD_ThroneRoom && ps.pending_throne_schedule_second_for.has_value() && ps.pending_throne_schedule_second_for.value() == to_replay) {
            ps.pending_throne_replay_stack.push_back(to_replay);
            ps.pending_throne_schedule_second_for.reset();
          }
          break;
        }
      }
      return;
    }
  }
  if (phase_ == Phase::actionPhase) {
    if (action_id == ActionIds::EndActions()) {
      phase_ = Phase::buyPhase;
      return;
    }
    if (action_id < ActionIds::BuyBase() && actions_ > 0 && action_id < static_cast<Action>(ps.hand_.size())) {
      CardName cn = ps.hand_[action_id];
      const Card& spec = GetCardSpec(cn);
      if (HasType(spec, CardType::ACTION)) {
        play_area_.push_back(cn);
        ps.hand_.erase(ps.hand_.begin() + action_id);
        actions_ -= 1;
        spec.play(*this, current_player_);
        spec.applyEffect(*this, current_player_);
      }
    }
  } else if (phase_ == Phase::buyPhase) {
    if (action_id == ActionIds::EndBuy()) {
      // Perform cleanup and start next turn (no explicit cleanup phase).
      last_player_to_go_ = current_player_;
      auto move_all = [&](std::vector<CardName>& from) {
        for (auto c : from) ps.discard_.push_back(c);
        from.clear();
      };
      move_all(ps.hand_);
      move_all(play_area_);
      coins_ = 0;
      actions_ = 1;
      buys_1 = 1;
      turn_number_ += 1;
      current_player_ = 1 - current_player_;
      phase_ = Phase::actionPhase;
      DrawCardsFor(current_player_, 5);
      return;
    }
    if (action_id < ActionIds::BuyBase() && action_id < static_cast<Action>(ps.hand_.size())) {
      CardName cn = ps.hand_[action_id];
      const Card& spec = GetCardSpec(cn);
      if (HasType(spec, CardType::TREASURE)) {
        play_area_.push_back(cn);
        ps.hand_.erase(ps.hand_.begin() + action_id);
        spec.play(*this, current_player_);
      }
      return;
    }
    if (action_id >= ActionIds::BuyBase() && action_id < ActionIds::BuyBase() + kNumSupplyPiles && buys_1 > 0) {
      int j = action_id - ActionIds::BuyBase();
      if (supply_piles_[j] > 0) {
        const Card& spec = GetCardSpec(supply_types_[j]);
        if (coins_ >= spec.cost_) {
          coins_ -= spec.cost_;
          buys_1 -= 1;
          ps.discard_.push_back(supply_types_[j]);
          supply_piles_[j] -= 1;
        }
      }
      return;
    }
  }
}

}  // namespace dominion
}  // namespace open_spiel