// AttackOption.h
#pragma once

#include "IAttackOption.h"
#include "character/Character.h"
#include "StateManager.h"
#include <cmath>
#include <algorithm>

// Use enum directly from StateManager.h
using CharacterState::JUMPING;
using CharacterState::FALLING;
using CharacterState::IDLE;

// Use AttackType values
using AttackType::JAB;
using AttackType::FORWARD_TILT;
using AttackType::UP_TILT;
using AttackType::DOWN_TILT;
using AttackType::DASH_ATTACK;
using AttackType::FORWARD_SMASH;
using AttackType::UP_SMASH;
using AttackType::DOWN_SMASH;
using AttackType::NEUTRAL_AIR;
using AttackType::FORWARD_AIR;
using AttackType::BACK_AIR;
using AttackType::UP_AIR;
using AttackType::DOWN_AIR;
using AttackType::NEUTRAL_SPECIAL;
using AttackType::SIDE_SPECIAL;
using AttackType::UP_SPECIAL;
using AttackType::DOWN_SPECIAL;
using AttackType::GRAB;
using AttackType::DOWN_THROW;

// Base class for all attack options
class AttackOption : public IAttackOption
{
public:
    virtual int GetAttackType() const override { return attackType; }

protected:
    int attackType;
};

// JabAttack implementation
class JabAttack : public AttackOption
{
public:
    JabAttack()
    {
        attackType = JAB;
    }

    float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) override
    {
        // Base utility for jab
        float utility = 0.5f;

        // Distance factors
        float optimalDistance = 60.0f;
        float distanceFactor = 1.0f - std::min(1.0f, std::fabs(std::fabs(distanceX) - optimalDistance) / 50.0f);
        utility *= distanceFactor;

        // Vertical position factor
        if (std::fabs(distanceY) > 40.0f)
        {
            utility *= 0.5f; // Jab is less effective if there's vertical separation
        }

        // Better when player is at low damage (for combo starters)
        if (player->damagePercent < 45.0f)
        {
            utility *= 1.2f;
        }

        // Better if player is grounded and facing us
        if (player->stateManager.state != JUMPING && player->stateManager.state != FALLING)
        {
            utility *= 1.3f;
        }

        // Less useful when player is shielding
        if (player->stateManager.isShielding)
        {
            utility *= 0.3f;
        }

        return std::min(1.0f, utility);
    }

    void Execute(Character* enemy) override
    {
        enemy->jab();
    }

    bool IsViable(float distanceX, float distanceY, Character* enemy) override
    {
        // Check if enemy is in an appropriate state to jab
        if (enemy->stateManager.state == JUMPING || enemy->stateManager.state == FALLING)
        {
            return false;
        }

        // Check if within jab range
        return std::fabs(distanceX) < 80.0f && std::fabs(distanceY) < 40.0f;
    }
};

// ForwardTiltAttack implementation
class ForwardTiltAttack : public AttackOption
{
public:
    ForwardTiltAttack()
    {
        attackType = FORWARD_TILT;
    }

    float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) override
    {
        // Base utility
        float utility = 0.6f;

        // Distance factors
        float optimalDistance = 90.0f;
        float distanceFactor = 1.0f - std::min(1.0f, std::fabs(std::fabs(distanceX) - optimalDistance) / 60.0f);
        utility *= distanceFactor;

        // Vertical position factor
        if (std::fabs(distanceY) > 50.0f)
        {
            utility *= 0.4f; // Forward tilt is less effective with vertical separation
        }

        // Better at mid damage
        if (player->damagePercent > 40.0f && player->damagePercent < 90.0f)
        {
            utility *= 1.3f;
        }

        // Better if player is grounded
        if (player->stateManager.state != JUMPING && player->stateManager.state != FALLING)
        {
            utility *= 1.2f;
        }

        // Less useful against shields
        if (player->stateManager.isShielding)
        {
            utility *= 0.4f;
        }

        return std::min(1.0f, utility);
    }

    void Execute(Character* enemy) override
    {
        enemy->forwardTilt();
    }

    bool IsViable(float distanceX, float distanceY, Character* enemy) override
    {
        // Not viable in air
        if (enemy->stateManager.state == JUMPING || enemy->stateManager.state == FALLING)
        {
            return false;
        }

        // Check if within range
        return std::fabs(distanceX) < 110.0f && std::fabs(distanceY) < 50.0f;
    }
};

// UpTiltAttack implementation
class UpTiltAttack : public AttackOption
{
public:
    UpTiltAttack()
    {
        attackType = UP_TILT;
    }

    float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) override
    {
        // Base utility
        float utility = 0.6f;

        // Distance factors - up tilt is better when player is above
        float horizontalFactor = 1.0f - std::min(1.0f, std::fabs(distanceX) / 70.0f);
        utility *= horizontalFactor;

        // Vertical position factor - better when player is above
        if (distanceY < 0 && std::fabs(distanceY) < 120.0f)
        {
            utility *= 1.5f; // Great for hitting above
        }
        else
        {
            utility *= 0.3f; // Poor if player isn't above
        }

        // Great combo starter at low percents
        if (player->damagePercent < 50.0f)
        {
            utility *= 1.4f;
        }

        // Less useful against shields
        if (player->stateManager.isShielding)
        {
            utility *= 0.5f;
        }

        return std::min(1.0f, utility);
    }

    void Execute(Character* enemy) override
    {
        enemy->upTilt();
    }

    bool IsViable(float distanceX, float distanceY, Character* enemy) override
    {
        // Not viable in air
        if (enemy->stateManager.state == JUMPING || enemy->stateManager.state == FALLING)
        {
            return false;
        }

        // Check if within range
        return std::fabs(distanceX) < 70.0f && distanceY > -140.0f && distanceY < 30.0f;
    }
};

// DownTiltAttack implementation
class DownTiltAttack : public AttackOption
{
public:
    DownTiltAttack()
    {
        attackType = DOWN_TILT;
    }

    float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) override
    {
        // Base utility
        float utility = 0.65f;

        // Distance factors
        float horizontalFactor = 1.0f - std::min(1.0f, std::fabs(distanceX) / 80.0f);
        utility *= horizontalFactor;

        // Vertical position factor - better when on same level
        if (std::fabs(distanceY) < 30.0f)
        {
            utility *= 1.4f; // Great for hitting low
        }
        else if (distanceY > 0)
        {
            utility *= 0.3f; // Poor if player is above
        }

        // Good combo starter
        if (player->damagePercent > 20.0f && player->damagePercent < 70.0f)
        {
            utility *= 1.3f;
        }

        // Can hit under shields sometimes
        if (player->stateManager.isShielding)
        {
            utility *= 0.7f; // Not terrible against shields
        }

        return std::min(1.0f, utility);
    }

    void Execute(Character* enemy) override
    {
        enemy->downTilt();
    }

    bool IsViable(float distanceX, float distanceY, Character* enemy) override
    {
        // Not viable in air
        if (enemy->stateManager.state == JUMPING || enemy->stateManager.state == FALLING)
        {
            return false;
        }

        // Check if within range
        return std::fabs(distanceX) < 90.0f && std::fabs(distanceY) < 40.0f;
    }
};

// ForwardSmashAttack implementation
class ForwardSmashAttack : public AttackOption
{
public:
    ForwardSmashAttack()
    {
        attackType = FORWARD_SMASH;
    }

    float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) override
    {
        // Base utility
        float utility = 0.5f;

        // Distance factors
        float optimalDistance = 100.0f;
        float distanceFactor = 1.0f - std::min(1.0f, std::fabs(std::fabs(distanceX) - optimalDistance) / 50.0f);
        utility *= distanceFactor;

        // Vertical position factor
        if (std::fabs(distanceY) > 40.0f)
        {
            utility *= 0.4f; // Forward smash is less effective with vertical separation
        }

        // Much better at high damage for KOs
        if (player->damagePercent > 90.0f)
        {
            utility *= 1.5f + ((player->damagePercent - 90.0f) / 60.0f);
        }
        else
        {
            utility *= 0.5f; // Poor utility at low damage
        }

        // Better if player is in endlag
        if (player->stateManager.isAttacking && player->stateManager.attackFrame > player->stateManager.attackDuration *
            0.6f)
        {
            utility *= 1.4f;
        }

        // Terrible against shields
        if (player->stateManager.isShielding)
        {
            utility *= 0.2f;
        }

        // Better near edge for KOs
        float screenCenter = GameConfig::SCREEN_WIDTH / 2;
        if (std::abs(player->physics.position.x - screenCenter) > 250)
        {
            utility *= 1.3f;
        }

        return std::min(1.0f, utility);
    }

    void Execute(Character* enemy) override
    {
        // Charge more at higher percents
        float chargeTime = 10.0f + (enemy->damagePercent / 30.0f);
        chargeTime = std::min(chargeTime, 25.0f); // Cap charge time
        enemy->forwardSmash(chargeTime);
    }

    bool IsViable(float distanceX, float distanceY, Character* enemy) override
    {
        // Not viable in air
        if (enemy->stateManager.state == JUMPING || enemy->stateManager.state == FALLING)
        {
            return false;
        }

        // Check if within range
        return std::fabs(distanceX) < 130.0f && std::fabs(distanceY) < 50.0f;
    }
};

// Up Smash Attack implementation
class UpSmashAttack : public AttackOption
{
public:
    UpSmashAttack()
    {
        attackType = UP_SMASH;
    }

    float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) override
    {
        // Base utility
        float utility = 0.5f;

        // Distance factors
        float horizontalFactor = 1.0f - std::min(1.0f, std::fabs(distanceX) / 70.0f);
        utility *= horizontalFactor;

        // Vertical position factor - better when player is above
        if (distanceY < 0 && std::fabs(distanceY) < 150.0f)
        {
            utility *= 1.6f; // Great for hitting above
        }
        else
        {
            utility *= 0.3f; // Poor if player isn't above
        }

        // Better at high damage for KOs
        if (player->damagePercent > 80.0f)
        {
            utility *= 1.4f + ((player->damagePercent - 80.0f) / 70.0f);
        }

        // Poor against shields
        if (player->stateManager.isShielding)
        {
            utility *= 0.3f;
        }

        return std::min(1.0f, utility);
    }

    void Execute(Character* enemy) override
    {
        // Charge more at higher percents
        float chargeTime = 10.0f + (enemy->damagePercent / 30.0f);
        chargeTime = std::min(chargeTime, 20.0f); // Cap charge time
        enemy->upSmash(chargeTime);
    }

    bool IsViable(float distanceX, float distanceY, Character* enemy) override
    {
        // Not viable in air
        if (enemy->stateManager.state == JUMPING || enemy->stateManager.state == FALLING)
        {
            return false;
        }

        // Check if within range and player is above
        return std::fabs(distanceX) < 80.0f && distanceY > -170.0f && distanceY < 40.0f;
    }
};

// NeutralAir implementation
class NeutralAirAttack : public AttackOption
{
public:
    NeutralAirAttack()
    {
        attackType = NEUTRAL_AIR;
    }

    float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) override
    {
        // Base utility
        float utility = 0.6f;

        // Only viable in air
        if (enemy->stateManager.state != JUMPING && enemy->stateManager.state != FALLING)
        {
            return 0.0f;
        }

        // Distance factors - neutral air is good all around
        float distanceFactor = 1.0f - std::min(
            1.0f, (std::sqrt(distanceX * distanceX + distanceY * distanceY) / 100.0f));
        utility *= distanceFactor;

        // Good combo tool at low-mid percents
        if (player->damagePercent > 10.0f && player->damagePercent < 60.0f)
        {
            utility *= 1.3f;
        }

        // Good against shields with crossup potential
        if (player->stateManager.isShielding)
        {
            utility *= 0.8f;
        }

        return std::min(1.0f, utility);
    }

    void Execute(Character* enemy) override
    {
        enemy->neutralAir();
    }

    bool IsViable(float distanceX, float distanceY, Character* enemy) override
    {
        // Only viable in air
        if (enemy->stateManager.state != JUMPING && enemy->stateManager.state != FALLING)
        {
            return false;
        }

        // Check if within range
        return std::sqrt(distanceX * distanceX + distanceY * distanceY) < 100.0f;
    }
};

// ForwardAir implementation
class ForwardAirAttack : public AttackOption
{
public:
    ForwardAirAttack()
    {
        attackType = FORWARD_AIR;
    }

    float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) override
    {
        // Base utility
        float utility = 0.7f;

        // Only viable in air
        if (enemy->stateManager.state != JUMPING && enemy->stateManager.state != FALLING)
        {
            return 0.0f;
        }

        // Distance factors - forward air needs horizontal alignment
        float horizontalFactor = 0.0f;

        // Different factor based on facing
        if (enemy->stateManager.isFacingRight && distanceX > 0)
        {
            horizontalFactor = 1.0f - std::min(1.0f, std::fabs(distanceX - 80.0f) / 60.0f);
        }
        else if (!enemy->stateManager.isFacingRight && distanceX < 0)
        {
            horizontalFactor = 1.0f - std::min(1.0f, std::fabs(distanceX + 80.0f) / 60.0f);
        }
        else
        {
            // Wrong direction
            horizontalFactor = 0.2f;
        }

        utility *= horizontalFactor;

        // Vertical position factor
        if (std::fabs(distanceY) > 60.0f)
        {
            utility *= 0.6f;
        }

        // Great KO move at high percent
        if (player->damagePercent > 90.0f)
        {
            utility *= 1.4f;
        }

        return std::min(1.0f, utility);
    }

    void Execute(Character* enemy) override
    {
        enemy->forwardAir();
    }

    bool IsViable(float distanceX, float distanceY, Character* enemy) override
    {
        // Only viable in air
        if (enemy->stateManager.state != JUMPING && enemy->stateManager.state != FALLING)
        {
            return false;
        }

        // Check if within range
        return std::fabs(distanceX) < 120.0f && std::fabs(distanceY) < 80.0f;
    }
};

// BackAir implementation
class BackAirAttack : public AttackOption
{
public:
    BackAirAttack()
    {
        attackType = BACK_AIR;
    }

    float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) override
    {
        // Base utility
        float utility = 0.7f;

        // Only viable in air
        if (enemy->stateManager.state != JUMPING && enemy->stateManager.state != FALLING)
        {
            return 0.0f;
        }

        // Distance factors - back air needs backward horizontal alignment
        float horizontalFactor = 0.0f;

        // Different factor based on facing
        if (enemy->stateManager.isFacingRight && distanceX < 0)
        {
            horizontalFactor = 1.0f - std::min(1.0f, std::fabs(distanceX + 80.0f) / 60.0f);
        }
        else if (!enemy->stateManager.isFacingRight && distanceX > 0)
        {
            horizontalFactor = 1.0f - std::min(1.0f, std::fabs(distanceX - 80.0f) / 60.0f);
        }
        else
        {
            // Wrong direction
            horizontalFactor = 0.2f;
        }

        utility *= horizontalFactor;

        // Vertical position factor
        if (std::fabs(distanceY) > 60.0f)
        {
            utility *= 0.6f;
        }

        // Strong KO move
        if (player->damagePercent > 90.0f)
        {
            utility *= 1.5f;
        }

        // Great for edge guarding
        if (player->physics.position.x < GameConfig::BLAST_ZONE_LEFT + 200 || player->physics.position.x >
            GameConfig::BLAST_ZONE_RIGHT - 200)
        {
            utility *= 1.4f;
        }

        return std::min(1.0f, utility);
    }

    void Execute(Character* enemy) override
    {
        enemy->backAir();
    }

    bool IsViable(float distanceX, float distanceY, Character* enemy) override
    {
        // Only viable in air
        if (enemy->stateManager.state != JUMPING && enemy->stateManager.state != FALLING)
        {
            return false;
        }

        // Check if within range
        return std::fabs(distanceX) < 120.0f && std::fabs(distanceY) < 80.0f;
    }
};

// Factory class for creating attack options
class AttackOptionFactory
{
public:
    static std::vector<std::unique_ptr<IAttackOption>> CreateAllAttackOptions()
    {
        std::vector<std::unique_ptr<IAttackOption>> options;

        // Add all attack options
        options.push_back(std::make_unique<JabAttack>());
        options.push_back(std::make_unique<ForwardTiltAttack>());
        options.push_back(std::make_unique<UpTiltAttack>());
        options.push_back(std::make_unique<DownTiltAttack>());
        options.push_back(std::make_unique<ForwardSmashAttack>());
        options.push_back(std::make_unique<UpSmashAttack>());
        options.push_back(std::make_unique<NeutralAirAttack>());
        options.push_back(std::make_unique<ForwardAirAttack>());
        options.push_back(std::make_unique<BackAirAttack>());

        // Add more attacks as needed

        return options;
    }
};
