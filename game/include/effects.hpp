#pragma once

// Effects in EffectCategory can initiate state change. They can also be secondary effects.
enum class EffectCategory {
    Gain,
    Draw,
    Put,      // From {Hand, Deck, Discard} to {Hand, Deck, Discard}
    Reveal,
    Trash,
    Discard,
    Look,     // e.g. Sentry
    SetAside, // e.g. Library setting aside skipped actions
    ActivateCondition // e.g. Merchant activates "FirstTimePlay" condition
};

// Secondary Effects are state changes that can only occur conditionally due to a primary effect.
enum class SecondaryEffectCategory {
    Skip // to be applied to "Draw"
};

enum class Condition {
    FirstTimePlay, // e.g. Merchant
    HandSize,      // e.g. Militia, Library
    MayPlay        // e.g. Throne Room, Vassal
};

enum class EffectAttribute {
    Optional // e.g. Mine Trash, Library draw while skipping
};

// Placeholder for a future effect chain implementation.
class EffectChain {
public:
    EffectChain() = default;
};