#include "../include/board_state.hpp"
#include "../include/context.hpp"
#include <algorithm>
#include <random>
#include <stdexcept>

Player::Player(std::string name_) : name(std::move(name_)) {
    // Initial deck: 7 Copper, 3 Estate
    for (int i = 0; i < 7; ++i) deck.push_back("Copper");
    for (int i = 0; i < 3; ++i) deck.push_back("Estate");
}

void Player::refill_deck(const GameContext& context) {
    if (!discard_pile.empty()) {
        std::mt19937 rng(context.seed);
        std::shuffle(discard_pile.begin(), discard_pile.end(), rng);
        deck.insert(deck.end(), discard_pile.begin(), discard_pile.end());
        discard_pile.clear();
    }
}

void Player::shuffle_deck(const GameContext& context) {
    std::mt19937 rng(context.seed);
    std::shuffle(deck.begin(), deck.end(), rng);
}

void Player::draw(const GameContext& context, int num_cards) {
    if ((int)deck.size() < num_cards) {
        refill_deck(context);
    }
    if ((int)deck.size() < num_cards) {
        // Draw as many as possible
        draw(context, static_cast<int>(deck.size()));
        return;
    }

    // Move top num_cards to hand
    hand.insert(hand.end(), deck.begin(), deck.begin() + num_cards);
    deck.erase(deck.begin(), deck.begin() + num_cards);
}

void Player::resolve_conditions(const GameContext& /*context*/) {
    // No-op for now.
}

SupplyPiles::SupplyPiles(const std::vector<std::string>& card_names, int initial_size) {
    piles = InitialSupplyPiles();
    for (const auto& card : card_names) {
        piles[card] = initial_size;
    }
}

BoardState::BoardState(std::vector<Player> players_, const std::vector<std::string>& supply_cards)
    : players(std::move(players_)), supply_piles(supply_cards) {
    if (players.empty()) {
        throw std::invalid_argument("Game must have at least one player");
    }
}