#include "../../include/character/CharacterMovement.h"

using CharacterState::IDLE;
using CharacterState::RUNNING;
using CharacterState::JUMPING;
using CharacterState::FALLING;
using CharacterState::SHIELDING;
using CharacterState::DODGING;

namespace CharacterMovement
{
    void executeJump(Character* character)
    {
        if (!character->stateManager.isJumping && character->stateManager.state != JUMPING)
        {
            character->physics.velocity.y = GameConfig::JUMP_FORCE;
            character->stateManager.isJumping = true;
            character->stateManager.changeState(JUMPING);
        }
        else if (character->stateManager.hasDoubleJump)
        {
            executeDoubleJump(character);
        }
    }

    void executeDoubleJump(Character* character)
    {
        if (character->stateManager.hasDoubleJump)
        {
            character->physics.velocity.y = GameConfig::DOUBLE_JUMP_FORCE;
            character->stateManager.hasDoubleJump = false;
            character->stateManager.changeState(JUMPING);
        }
    }

    void executeMoveLeft(Character* character)
    {
        if (character->stateManager.state != SHIELDING && character->stateManager.state != DODGING)
        {
            // FIXED: Set velocity directly to speed, without any damage scaling
            character->physics.velocity.x = -character->speed;
            character->stateManager.isFacingRight = false;

            if (character->stateManager.state == IDLE)
            {
                character->stateManager.changeState(RUNNING);
            }
        }
    }

    void executeMoveRight(Character* character)
    {
        if (character->stateManager.state != SHIELDING && character->stateManager.state != DODGING)
        {
            // FIXED: Set velocity directly to speed, without any damage scaling
            character->physics.velocity.x = character->speed;
            character->stateManager.isFacingRight = true;

            if (character->stateManager.state == IDLE)
            {
                character->stateManager.changeState(RUNNING);
            }
        }
    }


    void executeFastFall(Character* character)
    {
        character->physics.fastFall();
    }

    void executeDropThroughPlatform(Character* character)
    {
        if (character->stateManager.state != JUMPING && character->stateManager.state != FALLING)
        {
            // Move character down slightly to avoid immediate re-collision
            character->physics.position.y += 5;

            // Apply a small downward velocity to ensure continued falling
            character->physics.velocity.y = 1.0f;

            // Change state to falling
            character->stateManager.changeState(FALLING);
        }
    }

    void executeShield(Character* character)
    {
        if (character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING &&
            !character->stateManager.isAttacking &&
            character->stateManager.shieldHealth > 0 &&
            !character->stateManager.isDodging)
        {
            character->stateManager.changeState(SHIELDING);
            character->stateManager.isShielding = true;

            // Immobilize the character
            character->physics.velocity.x = 0;
            character->physics.velocity.y = 0;
        }
    }

    void executeReleaseShield(Character* character)
    {
        if (character->stateManager.isShielding)
        {
            character->stateManager.isShielding = false;
            character->stateManager.changeState(IDLE);
        }
    }

    void executeSpotDodge(Character* character)
    {
        if (character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING &&
            !character->stateManager.dodgeCD.isActive())
        {
            character->stateManager.changeState(DODGING);
            character->stateManager.dodgeFrames = 0;
            character->stateManager.isDodging = true;

            // Stop movement during spot dodge
            character->physics.velocity.x = 0;
            character->physics.velocity.y = 0;
        }
    }

    void executeForwardDodge(Character* character)
    {
        if (character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING &&
            !character->stateManager.dodgeCD.isActive())
        {
            character->stateManager.changeState(DODGING);
            character->stateManager.dodgeFrames = 0;
            character->stateManager.isDodging = true;

            // Move in facing direction
            character->physics.velocity.x = character->stateManager.isFacingRight
                                                ? character->speed * 1.5f
                                                : -character->speed * 1.5f;
            character->physics.velocity.y = 0;
        }
    }

    void executeBackDodge(Character* character)
    {
        if (character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING &&
            !character->stateManager.dodgeCD.isActive())
        {
            character->stateManager.changeState(DODGING);
            character->stateManager.dodgeFrames = 0;
            character->stateManager.isDodging = true;

            character->physics.velocity.x = character->stateManager.isFacingRight
                                                ? -character->speed * 1.5f
                                                : character->speed * 1.5f;
            character->physics.velocity.y = 0;
        }
    }

    void executeAirDodge(Character* character, float dirX, float dirY)
    {
        if ((character->stateManager.state == JUMPING || character->stateManager.state == FALLING) &&
            !character->stateManager.dodgeCD.isActive())
        {
            character->stateManager.changeState(DODGING);
            character->stateManager.dodgeFrames = 0;
            character->stateManager.isDodging = true;

            // Normalize direction and apply speed
            float length = sqrtf(dirX * dirX + dirY * dirY);
            if (length > 0)
            {
                character->physics.velocity.x = (dirX / length) * character->speed * 1.5f;
                character->physics.velocity.y = (dirY / length) * character->speed * 1.5f;
            }
        }
    }
} // namespace CharacterMovement
