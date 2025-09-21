#ifndef CHARACTER_STATE_H
#define CHARACTER_STATE_H

// Character states
namespace CharacterState {
    enum State {
        IDLE,
        RUNNING,
        JUMPING,
        FALLING,
        ATTACKING,
        SHIELDING,
        DODGING,
        HITSTUN,
        DYING
    };
}

// Attack types
namespace AttackType {
    enum Type {
        NONE,
        // Ground attacks
        JAB,                // A
        FORWARD_TILT,       // Side + A
        UP_TILT,            // Up + A
        DOWN_TILT,          // Down + A
        DASH_ATTACK,        // Run + A

        // Smash attacks
        FORWARD_SMASH,      // Side + A (charged)
        UP_SMASH,           // Up + A (charged)
        DOWN_SMASH,         // Down + A (charged)

        // Aerial attacks
        NEUTRAL_AIR,        // A in air
        FORWARD_AIR,        // Forward + A in air
        BACK_AIR,           // Back + A in air
        UP_AIR,             // Up + A in air
        DOWN_AIR,           // Down + A in air

        // Special attacks
        NEUTRAL_SPECIAL,    // B
        SIDE_SPECIAL,       // Side + B
        UP_SPECIAL,         // Up + B (recovery)
        DOWN_SPECIAL,       // Down + B

        // Grabs and throws
        GRAB,               // Z or Shield + A
        PUMMEL,             // A while grabbing
        FORWARD_THROW,      // Forward while grabbing
        BACK_THROW,         // Back while grabbing
        UP_THROW,           // Up while grabbing
        DOWN_THROW          // Down while grabbing
    };
}

#endif // CHARACTER_STATE_H