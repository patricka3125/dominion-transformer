# AGENTS.md - Dominion-Transformer Project Guide

**Last Updated:** November 2025
**Project:** Dominion Card Game Implementation for OpenSpiel/AlphaZero Training
**Language:** C++17
**Framework:** OpenSpiel (DeepMind game framework)

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Quick Start](#quick-start)
3. [Project Structure](#project-structure)
4. [Core Architecture](#core-architecture)
5. [Build & Test](#build--test)
6. [Key Components](#key-components)
7. [Workflows & Patterns](#workflows--patterns)
8. [Important Files](#important-files)
9. [Common Tasks](#common-tasks)
10. [Known Issues & TODOs](#known-issues--todos)
11. [Design Rationale](#design-rationale)
12. [Performance & Optimization](#performance--optimization)
13. [Useful Commands](#useful-commands)

---

## Executive Summary

**Dominion-Transformer** is a competitive C++ implementation of the deck-building card game Dominion, designed as a reinforcement learning environment for AlphaZero-style training within OpenSpiel. The project implements a modular card effect system, complex state management with effect queues, and full OpenSpiel compatibility for game tree search and neural network training.

**Key Goals:**
- Full OpenSpiel compatibility for MCTS and policy training
- Handle complex card interactions (especially Throne Room chaining)
- Support imperfect information semantics (hidden opponent decks)
- Serialize complete game state for training pipeline integration

**Current Status:**
- 14 base set cards fully implemented (Cellar, Chapel, Festival, Laboratory, Market, Merchant, Militia, Moat, Remodel, Smithy, ThroneRoom, Village, Witch, Workshop)
- 2-player sequential play with sampled stochastic shuffling
- Full effect queue resolution system
- JSON serialization for MCTS checkpointing

---

## Quick Start

### Prerequisites
- C++17 compiler (AppleClang/Clang/GCC)
- CMake 3.15+
- OpenSpiel cloned locally (sibling directory recommended)

### Build Steps

1. **Set OpenSpiel root:**
   ```bash
   export OPEN_SPIEL_ROOT="/Users/bytedance/dominion/open_spiel"
   ```

2. **Configure and build:**
   ```bash
   cd game
   cmake --build build -j 8
   ```

3. **Run tests:**
   ```bash
   ./build/dominion_test
   ./build/dominion_cards_test
   ```

4. **Generate playthrough:**
   ```bash
   bash -lc 'source $OPEN_SPIEL_ROOT/venv/bin/activate && \
     $OPEN_SPIEL_ROOT/scripts/generate_new_playthrough.sh dominion'
   ```

---

## Project Structure

```
dominion-transformer/
├── README.md                          # High-level project overview
├── CLAUDE.md                          # This file - AI assistant guide
├── docs/
│   └── dominion-transformer-design-doc.md  # Comprehensive architecture doc
├── game/
│   ├── CMakeLists.txt                 # CMake build configuration
│   ├── BUILD.md                       # Detailed build instructions
│   ├── include/
│   │   ├── dominion.hpp               # Main game/state classes (OpenSpiel interface)
│   │   ├── cards.hpp                  # Card specs, registry, base class
│   │   ├── actions.hpp                # Action ID system and mapping
│   │   └── effects.hpp                # Effect node hierarchy (FIFO queue system)
│   └── src/
│       ├── dominion.cpp               # Game state implementation, legal actions, state transitions
│       ├── cards.cpp                  # Card registry, base card logic
│       ├── actions.cpp                # Action string naming utilities
│       ├── effects.cpp                # Effect node handlers for complex effects
│       ├── dominion_test.cpp          # Unit tests: game flow, state transitions
│       ├── cards_test.cpp             # Card-specific behavior tests
│       └── cards/                     # Per-card implementations
│           ├── cellar.cpp / cellar_test.cpp
│           ├── chapel.cpp / chapel_test.cpp
│           ├── militia.cpp / militia_test.cpp
│           ├── remodel.cpp / remodel_test.cpp
│           ├── throne_room.cpp / throne_room_test.cpp
│           ├── witch.cpp / witch_test.cpp
│           ├── workshop.cpp / workshop_test.cpp
│           └── [... other card implementations ...]
└── open_spiel/
    └── tests/
        ├── console_play_test.cc       # Interactive console player for manual testing
        └── console_play_test.h
```

---

## Core Architecture

### High-Level Design

The project follows OpenSpiel's game architecture with three main layers:

```
┌─────────────────────────────────────────┐
│     OpenSpiel Framework Interface       │
│  (Game, State, LegalActions, etc.)      │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│      DominionGame & DominionState       │
│  (Game rules, state transitions)        │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│  Cards, Effects, Actions, PlayerState   │
│  (Game mechanics, card effects, queues) │
└─────────────────────────────────────────┘
```

### Key Design Principles

1. **Separation of Concerns:** Card definitions (specs), effect resolution, and game state are decoupled
2. **Polymorphic Effects:** Each card with unique behavior has its own `EffectNode` subclass
3. **FIFO Effect Queue:** Complex interactions resolved in order via `deque<EffectNode>`
4. **Stateless Card Specs:** Card definitions are singletons; runtime state lives in EffectNodes and PlayerState
5. **Serializable State:** Complete game state can be JSON-serialized for MCTS and training
6. **Imperfect Information:** Proper handling of hidden opponent deck/hand information

---

## Build & Test

### CMake Configuration

The build system uses environment variable `OPEN_SPIEL_ROOT` to locate OpenSpiel:

```bash
# Option 1: Environment variable
export OPEN_SPIEL_ROOT="/path/to/open_spiel"
cmake -S game -B game/build

# Option 2: CMake argument
cmake -S game -B game/build -DOPEN_SPIEL_ROOT="/path/to/open_spiel"

# Option 3: In-source build
cmake -S game -B game -DOPEN_SPIEL_ROOT="$OPEN_SPIEL_ROOT"
cmake --build game -j 8
```

### Build Targets

| Target | Purpose |
|--------|---------|
| `dominion_cpp_game` | Static library with core game logic |
| `dominion_test` | Unit tests for game state and transitions |
| `dominion_cards_test` | Card-specific behavior tests |
| `dominion_console_play` | Interactive console player (manual testing) |

### Test Execution

```bash
# Run all tests
./build/dominion_test
./build/dominion_cards_test

# Run specific test (if using Google Test)
./build/dominion_test --gtest_filter="TestName*"
```

### Troubleshooting Build Issues

| Issue | Solution |
|-------|----------|
| "OpenSpiel headers not found" | Set `OPEN_SPIEL_ROOT` or check path exists |
| "Abseil headers not found" | Verify `OPEN_SPIEL_ROOT` points to top-level `open_spiel/` directory |
| "nlohmann/json.hpp not found" | Check `OPEN_SPIEL_ROOT/open_spiel/json/include/` exists |
| Linker errors | Ensure OpenSpiel library is built in `$OPEN_SPIEL_ROOT/build/` |

---

## Key Components

### 1. **DominionGame** (`include/dominion.hpp`, `src/dominion.cpp`)

OpenSpiel `Game` interface implementation. Responsibilities:
- Create new game instances
- Define game parameters (players: 2, max actions: ~200)
- Register with OpenSpiel's game registry

**Key Constants:**
```cpp
inline constexpr int kNumPlayers = 2;
inline constexpr int kDominionMaxDistinctActions = 4096;  // TODO: reduce to ~250
inline constexpr int kNumCardTypes = 33;
inline constexpr int kNumSupplyPiles = kNumCardTypes;
```

### 2. **DominionState** (`include/dominion.hpp`, `src/dominion.cpp`)

OpenSpiel `State` interface implementation. Contains:
- Game phase management (Action → Buy → Cleanup)
- Supply pile tracking
- Legal action computation
- State serialization/deserialization
- Turn progression and shuffling

**Key Methods:**
- `LegalActions()` - Computes valid actions considering pending effects
- `DoApplyAction()` - State transition logic
- `DrawCardsFor()` - Draw with auto-shuffle when deck empty
- `Serialize()` / `ToStruct()` - JSON serialization

**Critical Members:**
```cpp
int current_player_;                          // 0 or 1
int coins_, actions_, buys_;                  // Turn resources
Phase phase_;                                 // Action/Buy/Cleanup
std::vector<CardName> play_area_;             // Cards played this turn
std::array<PlayerState, 2> player_states_;    // Per-player state
bool shuffle_pending_;                        // Chance node for shuffling
```

### 3. **PlayerState** (`include/dominion.hpp`)

Per-player state container:
```cpp
struct PlayerState {
  std::vector<CardName> deck_;                         // Ordered deck (top = back)
  std::array<int, kNumSupplyPiles> hand_counts_{};     // Hand card counts
  std::array<int, kNumSupplyPiles> discard_counts_{};  // Discard card counts
  PendingChoice pending_choice;                        // Current effect choice type
  std::deque<std::unique_ptr<EffectNode>> effect_queue; // FIFO effect queue
  std::unique_ptr<ObservationState> obs_state;         // Info state for player

  // Helper methods (recently added)
  int HandCount(CardName card) const;
  int TotalHandSize() const;
  int TotalDiscardSize() const;
  void AddToHand(CardName card, int count = 1);
  bool RemoveFromHand(CardName card, int count = 1);
  void AddToDiscard(CardName card, int count = 1);
  void MoveHandToDiscard();
  EffectNode* FrontEffect();
};
```

### 4. **Card System** (`include/cards.hpp`, `src/cards.cpp`)

Base `Card` class with subclasses for each card type:

```cpp
class Card {
  std::string name_;           // e.g., "Cellar"
  CardName kind_;              // CARD_Cellar enum
  std::vector<CardType> types_; // e.g., {Action}
  int cost_, value_, vp_;       // Card stats
  int grant_action_, grant_draw_, grant_buy_;  // Immediate grants
  bool has_unique_effect_;      // Whether to queue an EffectNode

  virtual void applyEffect(DominionState&, int player) const;  // Overridden by subclasses

  // Type query methods (recently added)
  bool IsAction() const;
  bool IsTreasure() const;
  bool IsBasicTreasure() const;
  bool IsAttack() const;
  bool IsVictory() const;
};
```

**Card Registry Pattern:**
- Singleton `CardRegistry()` holds all card specifications
- `GetCardSpec(CardName)` retrieves card by enumeration
- Subclasses (CellarCard, ChapelCard, etc.) override `applyEffect()`

### 5. **Effect System** (`include/effects.hpp`, `src/effects.cpp`)

Effect nodes implement complex card interactions using **CRTP pattern** for automatic clone():

```cpp
class EffectNode {
public:
  virtual ~EffectNode() = default;
  virtual std::unique_ptr<EffectNode> clone() const = 0;

  // Handler for player actions while this effect is active
  std::function<bool(DominionState&, int player, Action)> on_action;

  // Optional selection state accessors
  virtual HandSelectionStruct* hand_selection() { return nullptr; }
  virtual GainFromBoardStruct* gain_from_board() { return nullptr; }
};

// CRTP base for automatic clone implementation
template<typename Derived>
class CloneableEffectNode : public EffectNode {
public:
  std::unique_ptr<EffectNode> clone() const override {
    auto cloned = std::make_unique<Derived>(static_cast<const Derived&>(*this));
    cloned->on_action = on_action;
    cloned->enforce_ascending = enforce_ascending;
    return cloned;
  }
};
```

**Subclasses:**
- `CellarEffectNode` - Discard and draw
- `ChapelEffectNode` - Trash up to 4 cards
- `MilitiaEffectNode` - Opponent discards to 3
- `ThroneRoomEffectNode` - Play action twice (chainable!)
- `RemodelTrashEffectNode` / `RemodelGainEffectNode` - Two-phase effect
- `WorkshopEffectNode` - Gain card ≤4 cost
- `WitchEffectNode` - Give curse to opponents

**EffectNodeFactory Pattern** (recently added):
```cpp
class EffectNodeFactory {
public:
  static std::unique_ptr<EffectNode> CreateHandSelectionEffect(
      CardName card, PendingChoice choice, const HandSelectionStruct* hs = nullptr);
  static std::unique_ptr<EffectNode> CreateGainEffect(CardName card, int max_cost);
  static std::unique_ptr<EffectNode> CreateThroneRoomEffect(int depth = 0);
};
```

**Selection Structures:**
```cpp
struct HandSelectionStruct {
  int target_hand_size;               // When to auto-finish
  int last_selected_original_index;    // Ascending constraint for fairness
  int selection_count;                 // Cards selected so far
  bool allow_finish_selection;         // Can finish early?
};

struct GainFromBoardStruct {
  int max_cost;  // Maximum cost of cards player can gain
};
```

### 6. **Action System** (`include/actions.hpp`, `src/actions.cpp`)

Action ID layout spans ~200 distinct actions:

```
[0-32]        PlayHandIndex(i)     - Play card type i from hand
[33-65]       DiscardHandSelect(i) - Discard card type i
[66]          DiscardHandSelectFinish
[67-99]       TrashHandSelect(i)   - Trash card type i
[100]         TrashHandSelectFinish
[101]         ThroneHandSelectFinish
[102]         EndActions            - Transition to Buy phase
[103-135]     BuyFromSupply(j)     - Buy card type j
[136]         EndBuy                - End buy phase
[137-169]     GainSelect(j)        - Gain card j (for effect gains)
[170]         Shuffle              - Chance action (RNG)
[171]         PlayNonTerminal      - Macro action (composite)
```

**Helper functions** (recently added):
```cpp
inline int ToIndex(CardName card) { return static_cast<int>(card); }
inline CardName ToCardName(int idx) { return static_cast<CardName>(idx); }
inline bool IsValidPileIndex(int idx) { return idx >= 0 && idx < kNumSupplyPiles; }
```

---

## Workflows & Patterns

### Turn Flow

```
1. Action Phase
   └─ While actions > 0 AND hand has action cards:
      ├─ Play action card (or macro action)
      ├─ Apply immediate grants (+actions, +coins, +cards, +buys)
      └─ Enqueue effect node if card has unique effect
   └─ When no actions or actions = 0:
      └─ Transition to Buy Phase

2. Buy Phase
   ├─ Auto-play basic treasures on any buy action
   └─ While buys > 0:
      └─ Buy card from supply (cost ≤ coins)
   └─ When buys = 0 or EndBuy:
      └─ Transition to Cleanup

3. Cleanup
   ├─ Move hand + play_area to discard
   ├─ Draw 5 cards (shuffle discard if deck empty)
   └─ Switch to next player

4. Chance Node (if needed)
   ├─ If deck is empty during draw:
      ├─ Set shuffle_pending_ = true
      ├─ CurrentPlayer() returns kChancePlayerId
      └─ Only legal action: ActionIds::Shuffle()
   └─ Resume drawing after shuffle
```

### Effect Queue Processing

1. When a card with unique effect is played, create `EffectNode` and push to `player_states_[p].effect_queue`
2. Set `pending_choice` to indicate what actions are available
3. Loop: Front node's `on_action` handler processes player choices
4. On completion: Pop node, clear `pending_choice`, proceed to next effect or normal flow

**Example: Cellar Effect Queue**
```
Player plays Cellar
├─ Create CellarEffectNode using EffectNodeFactory
├─ Push to effect_queue
├─ Set pending_choice = DiscardUpToCardsFromHand
├─ on_action handler waits for DiscardHandSelect(i) or DiscardHandSelectFinish
├─ Player selects cards to discard
├─ When DiscardHandSelectFinish: draw cards equal to discarded count, pop node
└─ Resume turn (next effect or actions phase)
```

### Complex Chain: Throne Room

Throne Room is recursively chainable:
```cpp
class ThroneRoomEffectNode {
  int throne_select_depth_ = 0;

  // Depth tracking:
  // depth=0: Initial state (effect queued)
  // depth=1: Selecting first action to play twice
  // depth=2+: If selected action is another Throne Room, recurse
};
```

**Flow:**
1. Play Throne Room → Create node via factory, depth=0
2. Call `StartChain()` → depth=1, pending_choice = PlayActionFromHand
3. Player selects action (e.g., Throne Room)
4. Call `StartChain()` again → depth=2
5. Player selects action (e.g., Village)
6. Play Village twice, depth-- until depth=0
7. Pop node, continue

**Note** - Throne room is for the most part untested and still needs refinement on implementation.

---

## Important Files

### Headers to Read First
1. **dominion.hpp** (~400 lines) - Game and State interfaces, PlayerState struct
2. **cards.hpp** (~200 lines) - Card base class, CardName enum, CardRegistry
3. **effects.hpp** (~250 lines) - EffectNode hierarchy, CRTP, factory, selection structs
4. **actions.hpp** (~100 lines) - Action ID system and string mapping

### Implementation Files
- **dominion.cpp** - Game state logic, legal actions, transitions
- **cards.cpp** - Card registry, base card implementations
- **effects.cpp** - Effect node handlers, factory implementation
- **cards/*.cpp** - Per-card implementations (high complexity in remodel, throne_room)

### Test Files
- **dominion_test.cpp** - Test game flow, phases, state transitions
- **cards_test.cpp** - Test card behavior in isolation
- **cards/*_test.cpp** - Per-card unit tests

### Documentation
- **dominion-transformer-design-doc.md** - Comprehensive architecture, UML diagrams, sequence diagrams
- **BUILD.md** - Build instructions with troubleshooting
- **README.md** - High-level overview and quick start
- **CLAUDE.md** - This file - AI assistant guide

---

## Common Tasks

### Adding a New Card

1. **Define card spec in cards.hpp:**
   ```cpp
   class MyCardCard : public Card {
     void applyEffect(DominionState&, int player) const override;
   };
   ```

2. **Implement in cards/mycard.cpp:**
   ```cpp
   void MyCardCard::applyEffect(DominionState& state, int player) const {
     // Simple effect: just grants (already handled by base class)

     // Complex effect: use factory to create EffectNode
     auto& ps = state.player_states_[player];
     ps.effect_queue.clear();
     auto n = EffectNodeFactory::CreateHandSelectionEffect(
         CardName::CARD_MyCard,
         PendingChoice::DiscardUpToCardsFromHand);
     ps.effect_queue.push_back(std::move(n));
     Card::InitHandSelection(state, player, ps.FrontEffect(), PendingChoice::DiscardUpToCardsFromHand);
     ps.FrontEffect()->on_action = MyCardCard::MyCardHandler;
   }
   ```

3. **Register in CardRegistry:**
   ```cpp
   CardRegistry::Instance().Register(
     CardName::CARD_MyCard,
     std::make_unique<MyCardCard>(...)
   );
   ```

4. **Add tests in cards/mycard_test.cpp**

### Adding a New Effect Node

1. **Define in effects.hpp:**
   ```cpp
   class MyEffectNode : public CloneableEffectNode<MyEffectNode> {
   public:
     MyEffectNode() = default;
     MyEffectNode(PendingChoice choice, const HandSelectionStruct* hs = nullptr);
     HandSelectionStruct* hand_selection() override { return &hand_; }
   private:
     HandSelectionStruct hand_;
   };
   ```

2. **Implement constructor in effects.cpp:**
   ```cpp
   MyEffectNode::MyEffectNode(PendingChoice choice, const HandSelectionStruct* hs) {
     enforce_ascending = true;
     if (hs) hand_ = *hs;
     if (choice == PendingChoice::DiscardUpToCardsFromHand) {
       on_action = MyCard::MyCardHandler;
     }
   }
   ```

3. **Add to EffectNodeFactory in effects.cpp:**
   ```cpp
   std::unique_ptr<EffectNode> EffectNodeFactory::CreateHandSelectionEffect(...) {
     // ...
     case CardName::CARD_MyCard:
       node = std::unique_ptr<EffectNode>(new MyEffectNode(choice, hs));
       break;
   }
   ```

4. **Create in card's applyEffect() using factory** (see above)

### Debugging State Transitions

**Use Serialize() for logging:**
```cpp
std::cout << state.Serialize() << std::endl;  // Full JSON dump
```

**Key debug points:**
- `LegalActions()` - Check if action is computed correctly
- `DoApplyAction()` - Trace state change
- `effect_queue` - Inspect pending effects via `FrontEffect()`
- `pending_choice` - Verify choice type matches available actions

### Running Interactive Tests

```bash
./build/dominion_console_play
```

Allows manual play with output of legal actions and state.

---

## Known Issues & TODOs

### Known Bugs (from README)

1. **Throne Room PlayNonTerminal resolver incomplete** (Low Priority)
   - Throne Room needs better heuristic for selecting non-terminal actions
   - Current macro action `PlayNonTerminal` doesn't interact well with Throne Room chaining

2. **Ascending index constraint logic** (Medium Priority)
   - Hand selection fairness constraint (last_selected_original_index) needs refinement
   - May be too restrictive or not clear in semantics

3. **Macro action with negative-draw cards** (High Priority)
   - `PlayNonTerminal` may incorrectly select cards with net negative card advantage
   - Needs filtering of effect-node-producing draws

### High-Priority TODOs

1. **Implement Observation Tensor** (Critical for neural training)
   - `ObservationTensorShape()` currently returns empty
   - Need to encode supply counts, hand counts, opponent sizes, phase, coins, actions, buys, pending_choice_type
   - Design must support MLP architecture

2. **Improve Macro Actions**
   - Current `PlayNonTerminal` heuristic is simplistic
   - Goal: Make pluggable for different training phases

### Medium-Priority TODOs

1. **Add Kingdom Configuration**
   - Currently hardcoded to specific 14 cards
   - Goal: Support arbitrary kingdom via game parameters

2. **Refactor existing code to use new helpers**
   - Many places still use direct array access instead of helper methods
   - Gradually migrate to `TotalHandSize()`, `AddToHand()`, etc.

3. **Add Reaction Card Support**
   - Implement Moat defense against attacks
   - Model reaction phase in effect resolution

### Low-Priority TODOs

1. **Sparse Hand Representation**
   - Replace `array<int, 33>` with `map<CardName, int>`
   - Trade-off: Slightly more complex indexing for faster iteration

2. **Legal Action Caching**
   - Cache results with dirty flag
   - We can potentially cache in state struct.
   - MCTS calls `LegalActions()` repeatedly

3. **Plugin-Based Card Loading**
   - Support runtime loading of expansion packs
   - Enable extension without recompilation

---

## Design Rationale

### Why OpenSpiel Integration?

**OpenSpiel provides:**
- Standard game interface for MCTS and neural training
- Serialization framework for game state
- Pre-built bots and evaluation infrastructure
- Research-grade game collection

**Trade-off:** Must respect OpenSpiel's assumptions (2 players, sequential play, perfect recall, etc.)

---

## Performance & Optimization

### Recent Optimizations (Implemented)

1. **CRTP for clone()** - Eliminated 63 lines of boilerplate
2. **Helper methods** - Encapsulated common operations (TotalHandSize, FrontEffect, etc.)
3. **Index conversion helpers** - Reduced verbose static_cast operations
4. **EffectNodeFactory** - Centralized creation logic, cleaner code


### Optimization Opportunities (Priority Order)

| Priority | Task | Est. Impact | Effort |
|----------|------|-------------|--------|
| High | Observation tensor implementation | Critical | Medium |
| Medium | Extract LegalActions() logic | Readability | Medium |
| Low | Plugin card loading | Extensibility | High |
| Low | Legal Actions Caching | Performance (Medium) | Low |

### Memory Footprint

Current per-game-state memory:
- Supply piles: 33 × 4 bytes = 132 bytes
- Hand/discard counts: 2 × 33 × 2 × 4 bytes = 528 bytes
- Deck vector: ~10-20 cards × 4 bytes = 40-80 bytes
- Effect queue: Variable (typically 0-2 nodes)
- **Total:** ~1-2 KB per game state (very efficient)

---

## Useful Commands

### Build and Test

```bash
# Quick build and test
export OPEN_SPIEL_ROOT="/Users/bytedance/dominion/open_spiel"
cd game
cmake -S . -B build -DOPEN_SPIEL_ROOT="$OPEN_SPIEL_ROOT"
cmake --build build -j 8
./build/dominion_test && ./build/dominion_cards_test

# Rebuild (clean + build)
rm -rf build
cmake -S . -B build -DOPEN_SPIEL_ROOT="$OPEN_SPIEL_ROOT"
cmake --build build -j 8

# Build with specific cmake binary
/Users/bytedance/homebrew/bin/cmake --build build -j 8
```

### Git Workflow

```bash
# Check current branch and status
git status

# Create feature branch
git checkout -b feature/my-feature

# Commit with context
git add .
git commit -m "feat: Add observation tensor implementation"

# Push to origin
git push origin feature/my-feature
```

### Interactive Play

```bash
./build/dominion_console_play
```

Play manually and observe legal actions at each step.

### Code Inspection

```bash
# Find all references to a function
grep -r "DrawCardsFor" game/include
grep -r "DrawCardsFor" game/src

# Find all EffectNode subclasses
grep -r "class.*EffectNode" game/include

# Find all card registrations
grep -r "CARD_" game/include/cards.hpp
```

---

## References

**Primary Documentation:**
- `docs/dominion-transformer-design-doc.md` - Comprehensive architecture with UML and sequence diagrams
- `game/BUILD.md` - Build and troubleshooting guide
- `README.md` - Project overview and quick start

**OpenSpiel Integration:**
- OpenSpiel framework: https://github.com/deepmind/open_spiel
- Game interface: `open_spiel/spiel.h` → `Game`, `State` classes
- Game registration: OpenSpiel registry documentation

**Code Entry Points:**
- Game initialization: `DominionGame::NewInitialState()`
- State transitions: `DominionState::DoApplyAction()`
- Legal actions: `DominionState::LegalActions()`
- Effect queue processing: `PlayerState::effect_queue` and `on_action` handlers
- Effect node creation: `EffectNodeFactory::Create*()` methods

---

**Document Version:** 1.0
**Last Updated:** November 30, 2025
**For Questions/Updates:** Reference dominion-transformer-design-doc.md or project README
