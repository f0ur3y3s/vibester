#include "../../include/character/CharacterStateManager.h"

void CharacterStateManager::changeState(CharacterState newState)
{
    // Don't change state if in hitstun
    if (state == CharacterState::HITSTUN && newState != CharacterState::DYING && hitstunFrames > 0)
    {
        return;
    }

    // Don't change state if attacking (unless finished, hit, or dying)
    if (state == CharacterState::ATTACKING && newState != CharacterState::HITSTUN &&
        newState != CharacterState::DYING && attackFrame < attackDuration && isAttacking)
    {
        return;
    }

    // Handle state-specific transitions
    switch (newState)
    {
    case CharacterState::IDLE:
        break;

    case CharacterState::JUMPING:
        isJumping = true;
        break;

    case CharacterState::SHIELDING:
        isShielding = true;
        break;

    case CharacterState::DODGING:
        isDodging = true;
        dodgeFrames = 0;
        break;

    case CharacterState::HITSTUN:
        isHitstun = true;
        break;

    case CharacterState::DYING:
        isDying = true;
        break;

    default:
        break;
    }

    state = newState;
}

bool CharacterStateManager::canChangeState(CharacterState newState) const
{
    // Check for restrictions on state changes
    if (state == CharacterState::HITSTUN && hitstunFrames > 0 && newState != CharacterState::DYING)
    {
        return false;
    }

    if (state == CharacterState::ATTACKING && isAttacking && attackFrame < attackDuration &&
        newState != CharacterState::HITSTUN && newState != CharacterState::DYING)
    {
        return false;
    }

    return true;
}

bool CharacterStateManager::isAirborne() const
{
    return state == CharacterState::JUMPING || state == CharacterState::FALLING;
}

bool CharacterStateManager::isActionable() const
{
    return !isHitstun && !isDodging && !isDying && !isExploding;
}

void CharacterStateManager::updateTimers()
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
            state = CharacterState::IDLE;

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
            state = CharacterState::IDLE;
        }
    }
}

void CharacterStateManager::updateCooldowns()
{
    specialNeutralCD.update();
    specialSideCD.update();
    specialUpCD.update();
    specialDownCD.update();
    dodgeCD.update();
}
