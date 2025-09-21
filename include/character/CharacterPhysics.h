#ifndef CHARACTER_PHYSICS_H
#define CHARACTER_PHYSICS_H

#include "raylib.h"
#include "../CharacterConfig.h"
#include <cmath>

class CharacterPhysics
{
public:
    CharacterPhysics()
        : position({0, 0}), velocity({0, 0}), isFastFalling(false)
    {
    }

    CharacterPhysics(float x, float y)
        : position({x, y}), velocity({0, 0}), isFastFalling(false)
    {
    }

    // Core physics properties
    Vector2 position;
    Vector2 velocity;
    bool isFastFalling;

    // Apply gravity based on fast fall state
    void applyGravity()
    {
        if (isFastFalling)
        {
            velocity.y += GameConfig::FAST_FALL_GRAVITY;
        }
        else
        {
            velocity.y += GameConfig::GRAVITY;
        }
    }

    // Apply horizontal friction (different on ground vs. air)
    void applyFriction(bool onGround)
    {
        if (onGround)
        {
            // FIXED: Remove any scaling with damage percentage
            // Original code might have been:
            // float frictionFactor = BASE_FRICTION_FACTOR * (1.0f - (characterDamage / MAX_DAMAGE));

            velocity.x *= GameConfig::GROUND_FRICTION;

            // If velocity is very small, just stop completely to prevent sliding
            if (std::fabs(velocity.x) < 0.1f)
            {
                velocity.x = 0.0f;
            }
        }
        else
        {
            // While airborne, apply a much smaller amount of air resistance
            const float AIR_RESISTANCE = 0.98f;
            velocity.x *= AIR_RESISTANCE;
        }
    }

    // Set vertical velocity for jumping
    void jump(float force)
    {
        velocity.y = force;
    }

    // Enable fast falling
    void fastFall()
    {
        if (velocity.y > 0)
        {
            // Only if already falling
            isFastFalling = true;
            velocity.y = std::max(velocity.y, 5.0f); // Minimum fast fall speed
        }
    }

    // Cancel fast falling
    void cancelFastFall()
    {
        isFastFalling = false;
    }

    // Move horizontally with specified speed
    void moveHorizontal(float speed, bool facingRight)
    {
        velocity.x = facingRight ? speed : -speed;
    }

    // Stop horizontal movement
    void stopHorizontal()
    {
        velocity.x = 0.0f;
    }

    // Cap vertical velocity to prevent extreme values
    void capVerticalVelocity(float maxVelocity)
    {
        if (velocity.y > maxVelocity)
        {
            velocity.y = maxVelocity;
        }
        else if (velocity.y < -maxVelocity)
        {
            velocity.y = -maxVelocity;
        }
    }

    // Move position by current velocity
    void updatePosition()
    {
        position.x += velocity.x;
        position.y += velocity.y;
    }

    // Move position by partial velocity (for collision detection)
    void updatePositionPartial(float stepX, float stepY)
    {
        position.x += stepX;
        position.y += stepY;
    }
};

#endif // CHARACTER_PHYSICS_H
