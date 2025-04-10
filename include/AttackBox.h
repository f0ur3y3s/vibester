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

    // Constructor for standard attack
    AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur) {
        rect = r;
        damage = dmg;
        baseKnockback = baseKB;
        knockbackScaling = kbScaling;
        knockbackAngle = kbAngle;
        hitstunFrames = hitstun;
        duration = dur;
        currentFrame = 0;
        type = NORMAL;
        canSpike = false;
        shieldStun = 0;
        ignoresShield = false;
        causesFreeze = false;
        freezeFrames = 0;
        launchesUpward = false;
        knockbackGrowth = 0.0f;
        isActive = true;
        velocity = {0, 0};
        destroyOnHit = false;
    }

    // Constructor for projectile
    AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur,
              Vector2 vel, bool destroy = false) {
        rect = r;
        damage = dmg;
        baseKnockback = baseKB;
        knockbackScaling = kbScaling;
        knockbackAngle = kbAngle;
        hitstunFrames = hitstun;
        duration = dur;
        currentFrame = 0;
        type = PROJECTILE;
        canSpike = false;
        shieldStun = 0;
        ignoresShield = false;
        causesFreeze = false;
        freezeFrames = 0;
        launchesUpward = false;
        knockbackGrowth = 0.0f;
        isActive = true;
        velocity = vel;
        destroyOnHit = destroy;
    }

    // Constructor for special hitbox types
    AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur,
              BoxType hitType) {
        rect = r;
        damage = dmg;
        baseKnockback = baseKB;
        knockbackScaling = kbScaling;
        knockbackAngle = kbAngle;
        hitstunFrames = hitstun;
        duration = dur;
        currentFrame = 0;
        type = hitType;
        canSpike = false;
        shieldStun = 0;
        ignoresShield = (type == COMMAND_GRAB);
        causesFreeze = false;
        freezeFrames = 0;
        launchesUpward = false;
        knockbackGrowth = 0.0f;
        isActive = true;
        velocity = {0, 0};
        destroyOnHit = false;
    }

    // Methods
    bool update() {
        // For projectiles, update position
        if (type == PROJECTILE) {
            rect.x += velocity.x;
            rect.y += velocity.y;
        }

        currentFrame++;
        if (currentFrame >= duration) {
            isActive = false;
            return false;
        }

        return true;
    }

    void updatePosition(Vector2 ownerPos, bool isFacingRight) {
        if (type == PROJECTILE) {
            // Projectiles move independently once created
            return;
        }

        // Calculate offset based on facing direction
        float offsetX = isFacingRight ? 1.0f : -1.0f;
        float boxCenterX = ownerPos.x + (rect.width / 2) * offsetX;

        // Adjust the rect position
        rect.x = boxCenterX - (rect.width / 2);
        rect.y = ownerPos.y - (rect.height / 2);
    }

    void draw(bool debugMode = false) {
        if (!debugMode) return;

        Color hitboxColor;
        switch (type) {
            case GRAB:
            case COMMAND_GRAB:
                hitboxColor = {255, 100, 255, 128};  // Purple
                break;
            case PROJECTILE:
                hitboxColor = {0, 255, 255, 128};    // Cyan
                break;
            case REFLECTOR:
                hitboxColor = {0, 200, 255, 128};    // Blue
                break;
            case ABSORBER:
                hitboxColor = {0, 255, 200, 128};    // Teal
                break;
            case WINDBOX:
                hitboxColor = {200, 255, 200, 128};  // Light green
                break;
            default:
                hitboxColor = {255, 0, 0, 128};      // Red
                break;
        }

        DrawRectangleRec(rect, hitboxColor);
        DrawRectangleLinesEx(rect, 2, {255, 255, 255, 200});
    }

    // Calculate final knockback based on target damage
    Vector2 calculateKnockback(float targetDamage, float chargeMultiplier = 1.0f) {
        float angle = knockbackAngle * DEG2RAD;
        float knockbackMagnitude = baseKnockback +
                                  (damage * targetDamage * 0.05f * knockbackScaling * chargeMultiplier);

        if (launchesUpward) {
            // Override angle to be upward
            angle = 270.0f * DEG2RAD;
        }

        return {
            cosf(angle) * knockbackMagnitude,
            sinf(angle) * knockbackMagnitude
        };
    }
};

#endif // ATTACKBOX_H