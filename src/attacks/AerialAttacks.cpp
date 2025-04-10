#include "../../include/attacks/AerialAttacks.h"
#include "../../include/character/CharacterState.h"

using CharacterState::JUMPING;
using CharacterState::FALLING;
using CharacterState::ATTACKING;
using AttackType::NEUTRAL_AIR;
using AttackType::FORWARD_AIR;
using AttackType::BACK_AIR;
using AttackType::UP_AIR;
using AttackType::DOWN_AIR;

namespace AerialAttacks {

void executeNeutralAir(Character* character) {
    if (character->stateManager.canAttack && 
        (character->stateManager.state == JUMPING || character->stateManager.state == FALLING)) {
        
        character->resetAttackState();
        character->stateManager.isAttacking = true;
        character->stateManager.currentAttack = NEUTRAL_AIR;
        character->stateManager.attackDuration = 25;
        character->stateManager.changeState(ATTACKING);

        // Create circular hitbox around character
        float hitboxSize = character->width * 1.2f;
        float hitboxX = character->physics.position.x - hitboxSize / 2;
        float hitboxY = character->physics.position.y - hitboxSize / 2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxSize, hitboxSize};
        character->attacks.push_back(AttackBox(hitboxRect, 8.0f, 3.0f, 0.1f, 45.0f, 8, 8));
    }
}

void executeForwardAir(Character* character) {
    if (character->stateManager.canAttack && 
        (character->stateManager.state == JUMPING || character->stateManager.state == FALLING)) {
        
        character->resetAttackState();
        character->stateManager.isAttacking = true;
        character->stateManager.currentAttack = FORWARD_AIR;
        character->stateManager.attackDuration = 20;
        character->stateManager.changeState(ATTACKING);

        // Create forward air hitbox
        float hitboxWidth = character->width * 0.9f;
        float hitboxHeight = character->height * 0.5f;
        float hitboxX = character->stateManager.isFacingRight
                        ? character->physics.position.x + character->width / 2
                        : character->physics.position.x - character->width / 2 - hitboxWidth;
        float hitboxY = character->physics.position.y - hitboxHeight / 2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        character->attacks.push_back(AttackBox(hitboxRect, 9.0f, 4.0f, 0.15f, 
                                    character->stateManager.isFacingRight ? 45.0f : 135.0f, 12, 12));
    }
}

void executeBackAir(Character* character) {
    if (character->stateManager.canAttack && 
        (character->stateManager.state == JUMPING || character->stateManager.state == FALLING)) {
        
        character->resetAttackState();
        character->stateManager.isAttacking = true;
        character->stateManager.currentAttack = BACK_AIR;
        character->stateManager.attackDuration = 22;
        character->stateManager.changeState(ATTACKING);

        // Create back air hitbox
        float hitboxWidth = character->width * 0.9f;
        float hitboxHeight = character->height * 0.5f;
        float hitboxX = character->stateManager.isFacingRight
                        ? character->physics.position.x - character->width / 2 - hitboxWidth
                        : character->physics.position.x + character->width / 2;
        float hitboxY = character->physics.position.y - hitboxHeight / 2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        character->attacks.push_back(AttackBox(hitboxRect, 10.0f, 5.0f, 0.2f, 
                                    character->stateManager.isFacingRight ? 135.0f : 45.0f, 15, 15));
    }
}

void executeUpAir(Character* character) {
    if (character->stateManager.canAttack && 
        (character->stateManager.state == JUMPING || character->stateManager.state == FALLING)) {
        
        character->resetAttackState();
        character->stateManager.isAttacking = true;
        character->stateManager.currentAttack = UP_AIR;
        character->stateManager.attackDuration = 18;
        character->stateManager.changeState(ATTACKING);

        // Create up air hitbox
        float hitboxWidth = character->width * 0.8f;
        float hitboxHeight = character->height * 0.7f;
        float hitboxX = character->physics.position.x - hitboxWidth / 2;
        float hitboxY = character->physics.position.y - character->height / 2 - hitboxHeight;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        character->attacks.push_back(AttackBox(hitboxRect, 7.0f, 3.5f, 0.15f, 80.0f, 10, 10));
    }
}

void executeDownAir(Character* character) {
    if (character->stateManager.canAttack && 
        (character->stateManager.state == JUMPING || character->stateManager.state == FALLING)) {
        
        character->resetAttackState();
        character->stateManager.isAttacking = true;
        character->stateManager.currentAttack = DOWN_AIR;
        character->stateManager.attackDuration = 25;
        character->stateManager.changeState(ATTACKING);

        // Create down air hitbox
        float hitboxWidth = character->width * 0.6f;
        float hitboxHeight = character->height * 0.8f;
        float hitboxX = character->physics.position.x - hitboxWidth / 2;
        float hitboxY = character->physics.position.y + character->height / 2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        character->attacks.push_back(AttackBox(hitboxRect, 10.0f, 5.0f, 0.2f, 270.0f, 15, 15));
    }
}

} // namespace AerialAttacks