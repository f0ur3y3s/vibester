#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "CharacterConfig.h"
#include <string>

// Character states
enum CharacterState
{
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

// Attack types
enum AttackType
{
    NONE,
    // Ground attacks
    JAB, // A
    FORWARD_TILT, // Side + A
    UP_TILT, // Up + A
    DOWN_TILT, // Down + A
    DASH_ATTACK, // Run + A

    // Smash attacks
    FORWARD_SMASH, // Side + A (charged)
    UP_SMASH, // Up + A (charged)
    DOWN_SMASH, // Down + A (charged)

    // Aerial attacks
    NEUTRAL_AIR, // A in air
    FORWARD_AIR, // Forward + A in air
    BACK_AIR, // Back + A in air
    UP_AIR, // Up + A in air
    DOWN_AIR, // Down + A in air

    // Special attacks
    NEUTRAL_SPECIAL, // B
    SIDE_SPECIAL, // Side + B
    UP_SPECIAL, // Up + B (recovery)
    DOWN_SPECIAL, // Down + B

    // Grabs and throws
    GRAB, // Z or Shield + A
    PUMMEL, // A while grabbing
    FORWARD_THROW, // Forward while grabbing
    BACK_THROW, // Back while grabbing
    UP_THROW, // Up while grabbing
    DOWN_THROW // Down while grabbing
};

// Cooldown timer structure
struct Cooldown
{
    int duration;
    int current;

    Cooldown(int dur) : duration(dur), current(0)
    {
    }

    bool isActive() const { return current > 0; }
    void update() { if (current > 0) current--; }
    void reset() { current = duration; }
    void reset(float multiplier) { current = static_cast<int>(duration * multiplier); }
};

class StateManager
{
public:
    StateManager()
        : state(IDLE),
          isFacingRight(true),
          isJumping(false),
          hasDoubleJump(true),
          isAttacking(false),
          canAttack(true),
          isShielding(false),
          shieldHealth(GameConfig::MAX_SHIELD_HEALTH),
          isDodging(false),
          dodgeFrames(0),
          isHitstun(false),
          hitstunFrames(0),
          isInvincible(false),
          invincibilityFrames(0),
          isGrabbing(false),
          grabDuration(0),
          grabFrame(0),
          isDying(false),
          isExploding(false),
          currentAttack(NONE),
          attackDuration(0),
          attackFrame(0),
          deathFrame(0),
          deathDuration(60),
          explosionFrame(0),
          explosionDuration(60),
          specialNeutralCD(120),
          specialSideCD(90),
          specialUpCD(60),
          specialDownCD(120),
          dodgeCD(GameConfig::DODGE_COOLDOWN)
    {
    }

    void changeState(CharacterState newState)
    {
        // Don't change state if in hitstun
        if (state == HITSTUN && newState != DYING && hitstunFrames > 0)
        {
            return;
        }

        // Don't change state if attacking (unless finished, hit, or dying)
        if (state == ATTACKING && newState != HITSTUN && newState != DYING &&
            attackFrame < attackDuration && isAttacking)
        {
            return;
        }

        // Handle state-specific transitions
        switch (newState)
        {
        case IDLE:
            break;

        case JUMPING:
            isJumping = true;
            break;

        case SHIELDING:
            isShielding = true;
            break;

        case DODGING:
            isDodging = true;
            dodgeFrames = 0;
            break;

        case HITSTUN:
            isHitstun = true;
            break;

        case DYING:
            isDying = true;
            break;

        default:
            break;
        }

        state = newState;
    }

    bool canChangeState(CharacterState newState) const
    {
        // Check for restrictions on state changes
        if (state == HITSTUN && hitstunFrames > 0 && newState != DYING)
        {
            return false;
        }

        if (state == ATTACKING && isAttacking && attackFrame < attackDuration &&
            newState != HITSTUN && newState != DYING)
        {
            return false;
        }

        return true;
    }

    bool isAirborne() const
    {
        return state == JUMPING || state == FALLING;
    }

    bool isActionable() const
    {
        return !isHitstun && !isDodging && !isDying && !isExploding;
    }

    void updateTimers()
    {
        // Update invincibility
        if (isInvincible)
        {
            invincibilityFrames--;
            if (invincibilityFrames <= 0)
            {
                isInvincible = false;
            }
        }

        // Update hitstun
        if (isHitstun)
        {
            hitstunFrames--;
            if (hitstunFrames <= 0)
            {
                isHitstun = false;
            }
        }

        // Update dodge
        if (isDodging)
        {
            dodgeFrames++;

            // Set invincibility during frames
            if (dodgeFrames >= GameConfig::DODGE_INVINCIBLE_START &&
                dodgeFrames <= GameConfig::DODGE_INVINCIBLE_END)
            {
                isInvincible = true;
            }
            else
            {
                isInvincible = false;
            }

            // End dodge after duration
            if (dodgeFrames >= GameConfig::SPOT_DODGE_FRAMES)
            {
                isDodging = false;
                dodgeFrames = 0;
                isInvincible = false;

                // Change state based on circumstances
                state = IDLE;

                // Start cooldown
                dodgeCD.reset();
            }
        }

        // Update shield health
        if (isShielding)
        {
            shieldHealth = std::min(shieldHealth + GameConfig::SHIELD_REGEN_RATE,
                                    GameConfig::MAX_SHIELD_HEALTH);
        }

        // Update grab duration
        if (isGrabbing)
        {
            grabFrame++;
            if (grabFrame >= grabDuration)
            {
                isGrabbing = false;
                grabFrame = 0;

                // Return to idle state
                state = IDLE;
            }
        }
    }

    void updateCooldowns()
    {
        specialNeutralCD.update();
        specialSideCD.update();
        specialUpCD.update();
        specialDownCD.update();
        dodgeCD.update();
    }

    // Current state and flags
    CharacterState state;
    bool isFacingRight;
    bool isJumping;
    bool hasDoubleJump;
    bool isAttacking;
    bool canAttack;
    bool isShielding;
    float shieldHealth;
    bool isDodging;
    int dodgeFrames;
    bool isHitstun;
    int hitstunFrames;
    bool isInvincible;
    int invincibilityFrames;

    // Grab state
    bool isGrabbing;
    int grabDuration;
    int grabFrame;

    // Death and explosion state
    bool isDying;
    bool isExploding;
    int deathFrame;
    int deathDuration;
    int explosionFrame;
    int explosionDuration;

    // Attack state
    AttackType currentAttack;
    int attackDuration;
    int attackFrame;

    // Cooldowns
    Cooldown specialNeutralCD;
    Cooldown specialSideCD;
    Cooldown specialUpCD;
    Cooldown specialDownCD;
    Cooldown dodgeCD;
};

#endif // STATE_MANAGER_H
