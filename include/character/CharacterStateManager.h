#ifndef CHARACTER_STATE_MANAGER_H
#define CHARACTER_STATE_MANAGER_H

#include "../CharacterConfig.h"
#include "../StateManager.h"

#include <algorithm>

using ::CharacterState;
using ::AttackType ;

class CharacterStateManager {
public:
    CharacterStateManager()
        : state(CharacterState::IDLE),
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
          currentAttack(AttackType::NONE),
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
          dodgeCD(GameConfig::DODGE_COOLDOWN) {}

    void changeState(CharacterState newState);
    bool canChangeState(CharacterState newState) const;
    bool isAirborne() const;
    bool isActionable() const;
    void updateTimers();
    void updateCooldowns();

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

#endif // CHARACTER_STATE_MANAGER_H