#include "../../include/attacks/StandardAttacks.h"

using CharacterState::JUMPING;
using CharacterState::FALLING;
using CharacterState::ATTACKING;
using AttackType::JAB;
using AttackType::FORWARD_TILT;
using AttackType::UP_TILT;
using AttackType::DOWN_TILT;
using AttackType::DASH_ATTACK;
using AttackType::FORWARD_SMASH;
using AttackType::UP_SMASH;
using AttackType::DOWN_SMASH;

namespace StandardAttacks
{
    void executeJab(Character* character)
    {
        if (character->stateManager.canAttack &&
            character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING)
        {
            character->resetAttackState();
            character->stateManager.isAttacking = true;
            character->stateManager.currentAttack = JAB;
            character->stateManager.attackDuration = 15; // Very short duration
            character->stateManager.changeState(ATTACKING);

            // Small hitbox with minimal knockback
            float hitboxWidth = character->width * 0.7f;
            float hitboxHeight = character->height * 0.5f;
            float hitboxX = character->stateManager.isFacingRight
                                ? character->physics.position.x + character->width / 2
                                : character->physics.position.x - character->width / 2 - hitboxWidth;
            float hitboxY = character->physics.position.y - hitboxHeight / 2;

            Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
            character->attacks.push_back(AttackBox(hitboxRect, 3.0f, 1.5f, 0.05f,
                                                   character->stateManager.isFacingRight ? 0.0f : 180.0f, 5, 5));

            // Reduced end lag - can act again sooner
            character->stateManager.canAttack = true;
        }
    }

    void executeForwardTilt(Character* character)
    {
        if (character->stateManager.canAttack &&
            character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING)
        {
            character->resetAttackState();
            character->stateManager.isAttacking = true;
            character->stateManager.currentAttack = FORWARD_TILT;
            character->stateManager.attackDuration = 20;
            character->stateManager.changeState(ATTACKING);

            // Create forward tilt hitbox
            float hitboxWidth = character->width * 0.8f;
            float hitboxHeight = character->height * 0.4f;
            float hitboxX = character->stateManager.isFacingRight
                                ? character->physics.position.x + character->width / 2
                                : character->physics.position.x - character->width / 2 - hitboxWidth;
            float hitboxY = character->physics.position.y - character->height * 0.1f;

            Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
            character->attacks.push_back(AttackBox(hitboxRect, 6.0f, 3.5f, 0.1f,
                                                   character->stateManager.isFacingRight ? 30.0f : 150.0f, 8, 8));
        }
    }

    void executeUpTilt(Character* character)
    {
        if (character->stateManager.canAttack &&
            character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING)
        {
            character->resetAttackState();
            character->stateManager.isAttacking = true;
            character->stateManager.currentAttack = UP_TILT;
            character->stateManager.attackDuration = 18;
            character->stateManager.changeState(ATTACKING);

            // Create up tilt hitbox
            float hitboxWidth = character->width * 0.7f;
            float hitboxHeight = character->height * 0.8f;
            float hitboxX = character->physics.position.x - hitboxWidth / 2;
            float hitboxY = character->physics.position.y - character->height / 2 - hitboxHeight / 2;

            Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
            character->attacks.push_back(AttackBox(hitboxRect, 5.0f, 2.0f, 0.12f, 80.0f, 8, 8));
        }
    }

    void executeDownTilt(Character* character)
    {
        if (character->stateManager.canAttack &&
            character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING)
        {
            character->resetAttackState();
            character->stateManager.isAttacking = true;
            character->stateManager.currentAttack = DOWN_TILT;
            character->stateManager.attackDuration = 15;
            character->stateManager.changeState(ATTACKING);

            // Create down tilt hitbox
            float hitboxWidth = character->width * 1.0f;
            float hitboxHeight = character->height * 0.3f;
            float hitboxX = character->stateManager.isFacingRight
                                ? character->physics.position.x + character->width / 4
                                : character->physics.position.x - character->width / 4 - hitboxWidth;
            float hitboxY = character->physics.position.y + character->height / 2 - hitboxHeight;

            Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
            character->attacks.push_back(AttackBox(hitboxRect, 5.0f, 1.5f, 0.08f, 0.0f, 5, 5));
        }
    }

    void executeDashAttack(Character* character)
    {
        if (character->stateManager.canAttack)
        {
            character->resetAttackState();
            character->stateManager.isAttacking = true;
            character->stateManager.currentAttack = DASH_ATTACK;
            character->stateManager.attackDuration = 25;
            character->stateManager.changeState(ATTACKING);

            // Add momentum to dash attack
            character->physics.velocity.x = character->stateManager.isFacingRight
                                                ? character->speed * 1.5f
                                                : -character->speed * 1.5f;

            // Create dash attack hitbox
            float hitboxWidth = character->width * 1.0f;
            float hitboxHeight = character->height * 0.6f;
            float hitboxX = character->stateManager.isFacingRight
                                ? character->physics.position.x + character->width / 2
                                : character->physics.position.x - character->width / 2 - hitboxWidth;
            float hitboxY = character->physics.position.y - hitboxHeight / 2;

            Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
            character->attacks.push_back(AttackBox(hitboxRect, 7.0f, 4.0f, 0.15f,
                                                   character->stateManager.isFacingRight ? 30.0f : 150.0f, 10, 10));
        }
    }

    // Smash Attacks
    void executeForwardSmash(Character* character, float chargeTime)
    {
        if (character->stateManager.canAttack &&
            character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING)
        {
            character->resetAttackState();
            character->stateManager.isAttacking = true;
            character->stateManager.currentAttack = FORWARD_SMASH;
            character->stateManager.attackDuration = 30;
            character->stateManager.changeState(ATTACKING);

            // Charge multiplier (1.0 to 1.5)
            float chargeMultiplier = 1.0f + (chargeTime / 60.0f) * 0.5f;
            if (chargeMultiplier > 1.5f) chargeMultiplier = 1.5f;

            // Create forward smash hitbox
            float hitboxWidth = character->width * 1.2f;
            float hitboxHeight = character->height * 0.6f;
            float hitboxX = character->stateManager.isFacingRight
                                ? character->physics.position.x + character->width / 2
                                : character->physics.position.x - character->width / 2 - hitboxWidth;
            float hitboxY = character->physics.position.y - hitboxHeight / 2;

            Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
            character->attacks.push_back(AttackBox(
                hitboxRect,
                12.0f * chargeMultiplier,
                6.0f * chargeMultiplier,
                0.25f * chargeMultiplier,
                character->stateManager.isFacingRight ? 30.0f : 150.0f,
                15,
                15
            ));
        }
    }

    void executeUpSmash(Character* character, float chargeTime)
    {
        if (character->stateManager.canAttack &&
            character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING)
        {
            character->resetAttackState();
            character->stateManager.isAttacking = true;
            character->stateManager.currentAttack = UP_SMASH;
            character->stateManager.attackDuration = 30;
            character->stateManager.changeState(ATTACKING);

            // Charge multiplier (1.0 to 1.5)
            float chargeMultiplier = 1.0f + (chargeTime / 60.0f) * 0.5f;
            if (chargeMultiplier > 1.5f) chargeMultiplier = 1.5f;

            // Create up smash hitbox
            float hitboxWidth = character->width * 0.8f;
            float hitboxHeight = character->height * 1.2f;
            float hitboxX = character->physics.position.x - hitboxWidth / 2;
            float hitboxY = character->physics.position.y - character->height / 2 - hitboxHeight / 2;

            Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
            character->attacks.push_back(AttackBox(
                hitboxRect,
                13.0f * chargeMultiplier,
                7.0f * chargeMultiplier,
                0.2f * chargeMultiplier,
                90.0f,
                15,
                15
            ));
        }
    }

    void executeDownSmash(Character* character, float chargeTime)
    {
        if (character->stateManager.canAttack &&
            character->stateManager.state != JUMPING &&
            character->stateManager.state != FALLING)
        {
            character->resetAttackState();
            character->stateManager.isAttacking = true;
            character->stateManager.currentAttack = DOWN_SMASH;
            character->stateManager.attackDuration = 35;
            character->stateManager.changeState(ATTACKING);

            // Charge multiplier (1.0 to 1.5)
            float chargeMultiplier = 1.0f + (chargeTime / 60.0f) * 0.5f;
            if (chargeMultiplier > 1.5f) chargeMultiplier = 1.5f;

            // Create two hitboxes, one on each side
            float hitboxWidth = character->width * 0.8f;
            float hitboxHeight = character->height * 0.4f;

            // Left hitbox
            float leftHitboxX = character->physics.position.x - character->width / 2 - hitboxWidth;
            float hitboxY = character->physics.position.y + character->height / 2 - hitboxHeight;
            Rectangle leftHitboxRect = {leftHitboxX, hitboxY, hitboxWidth, hitboxHeight};

            // Right hitbox
            float rightHitboxX = character->physics.position.x + character->width / 2;
            Rectangle rightHitboxRect = {rightHitboxX, hitboxY, hitboxWidth, hitboxHeight};

            character->attacks.push_back(AttackBox(
                leftHitboxRect,
                11.0f * chargeMultiplier,
                5.5f * chargeMultiplier,
                0.2f * chargeMultiplier,
                20.0f,
                15,
                15
            ));

            character->attacks.push_back(AttackBox(
                rightHitboxRect,
                11.0f * chargeMultiplier,
                5.5f * chargeMultiplier,
                0.2f * chargeMultiplier,
                160.0f,
                15,
                15
            ));
        }
    }
} // namespace StandardAttacks
