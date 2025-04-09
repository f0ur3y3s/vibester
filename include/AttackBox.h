#ifndef ATTACKBOX_H
#define ATTACKBOX_H

#include "raylib.h"

// Attack hitbox - enhanced for Smash Bros style
struct AttackBox {
    Rectangle rect;
    float damage;                  // Percentage damage
    float baseKnockback;           // Base knockback applied
    float knockbackScaling;        // How much knockback scales with target's damage
    float knockbackGrowth;         // Knockback growth with charge
    float knockbackAngle;          // Angle in degrees (0 = right, 90 = up)
    int hitstunFrames;             // Frames of hitstun
    bool canSpike;                 // Can meteor smash/spike opponents downward
    bool ignoresShield;            // If true, goes through shields
    int shieldStun;                // Extra frames of stun when hitting shield
    int duration;                  // How long hitbox is active
    int currentFrame;
    bool isActive;                 // Whether hitbox is currently active

    // Effects
    bool causesFreeze;             // Freeze-frame effect on hit
    int freezeFrames;              // How many frames to freeze on hit
    bool launchesUpward;           // Forces upward knockback regardless of angle

    // Special type effects
    enum HitboxType {
        NORMAL,
        PROJECTILE,        // Moves independently
        ABSORBER,          // Can absorb projectiles
        REFLECTOR,         // Can reflect projectiles
        GRAB,              // Initiates grab if hits
        WINDBOX,           // Pushes without damage
        COMMAND_GRAB       // Grabs through shield
    };

    HitboxType type;

    // For projectiles
    Vector2 velocity;
    bool destroyOnHit;

    // Constructor for standard attack
    AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur);

    // Constructor for projectile
    AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur,
              Vector2 vel, bool destroy);

    // Constructor for special hitbox types
    AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur,
              HitboxType hitType);

    // Methods
    bool update();
    void updatePosition(Vector2 ownerPos, bool isFacingRight);
    void draw(bool debugMode = false);

    // Calculate final knockback based on target damage
    Vector2 calculateKnockback(float targetDamage, float chargeTime = 0.0f);
};

#endif // ATTACKBOX_H