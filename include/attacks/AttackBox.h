#ifndef ATTACKBOX_H
#define ATTACKBOX_H

#include "raylib.h"
#include <cmath>

// Attack hitbox - enhanced for Smash Bros style
class AttackBox {
public:
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
    enum BoxType {
        NORMAL,
        PROJECTILE,        // Moves independently
        ABSORBER,          // Can absorb projectiles
        REFLECTOR,         // Can reflect projectiles
        GRAB,              // Initiates grab if hits
        WINDBOX,           // Pushes without damage
        COMMAND_GRAB       // Grabs through shield
    };

    BoxType type;

    // For projectiles
    Vector2 velocity;
    bool destroyOnHit;
    AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur);
    AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur, Vector2 vel,
              bool destroy);
    AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur,
              BoxType hitType);
    bool update();
    void updatePosition(Vector2 ownerPos, bool isFacingRight);
    void draw(bool debugMode);
    Vector2 calculateKnockback(float targetDamage, float chargeMultiplier);
};

#endif // ATTACKBOX_H