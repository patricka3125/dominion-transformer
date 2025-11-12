#pragma once
#include <string>
#include <vector>
#include <unordered_map>

class GameContext; // forward declaration

// Represents the starting supply pile sizes for each basic card.
inline const std::unordered_map<std::string, int>& InitialSupplyPiles() {
    static const std::unordered_map<std::string, int> supply = {
        {"Copper",   60},
        {"Silver",   40},
        {"Gold",     30},
        {"Estate",    8},
        {"Duchy",     8},
        {"Province",  8},
        {"Curse",    10},
    };
    return supply;
}

// Player representation with deck/hand/discard, etc.
class Player {
public:
    std::string name;
    std::vector<std::string> hand;
    std::vector<std::string> deck;
    std::vector<std::string> discard_pile;
    std::vector<std::string> trash_pile;
    std::vector<std::string> play_history;
    std::vector<std::string> active_conditions;
    int vps = 0;

    explicit Player(std::string name_);

    // refill_deck shuffles the discard pile into the deck.
    void refill_deck(const GameContext& context);
    // Shuffle the deck using the context seed.
    void shuffle_deck(const GameContext& context);
    // Draw cards from the deck into the hand.
    void draw(const GameContext& context, int num_cards = 1);
    // Resolve active conditions (currently no-op).
    void resolve_conditions(const GameContext& context);
};

// Supply piles with initial sizes.
class SupplyPiles {
public:
    std::unordered_map<std::string, int> piles;
    explicit SupplyPiles(const std::vector<std::string>& card_names, int initial_size = 10);
};

// BoardState holds players and supply piles.
class BoardState {
public:
    std::vector<Player> players;
    SupplyPiles supply_piles;

    BoardState(std::vector<Player> players_, const std::vector<std::string>& supply_cards);
};