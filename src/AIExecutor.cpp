// AIExecutor.cpp
#include "AIExecutor.h"
#include "Character.h"
#include "Platform.h"
#include "Constants.h"
#include "AttackOption.h" // Include specific attack options
#include <algorithm>
#include <cmath>

// Use enums directly
using CharacterState::JUMPING;
using CharacterState::FALLING;
using CharacterState::IDLE;

// Use AttackType directly
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

AIExecutor::AIExecutor(AIConfig& config) : config(config), platforms(nullptr)
{
    // Initialize attack options
    attackOptions = AttackOptionFactory::CreateAllAttackOptions();
}

void AIExecutor::ExecuteAction(Character* enemy, Character* player, float distanceX, float distanceY, int actionId)
{
    EnhancedAIState::State action = static_cast<EnhancedAIState::State>(actionId);
    float absDistanceX = std::fabs(distanceX);
    float absDistanceY = std::fabs(distanceY);

    // Face the player
    enemy->stateManager.isFacingRight = (distanceX > 0);

    // Execute the appropriate behavior based on state
    switch (action)
    {
    case EnhancedAIState::NEUTRAL:
        ExecuteNeutralBehavior(enemy, player, distanceX, distanceY);
        break;

    case EnhancedAIState::APPROACH:
        ExecuteApproachBehavior(enemy, player, distanceX, distanceY);
        break;

    case EnhancedAIState::ATTACK:
        ExecuteAttackBehavior(enemy, player, distanceX, distanceY);
        break;

    case EnhancedAIState::PRESSURE:
        ExecutePressureBehavior(enemy, player, distanceX, distanceY);
        break;

    case EnhancedAIState::BAIT:
        ExecuteBaitBehavior(enemy, player, distanceX, distanceY);
        break;

    case EnhancedAIState::DEFEND:
        ExecuteDefendBehavior(enemy, player, distanceX, distanceY);
        break;

    case EnhancedAIState::PUNISH:
        ExecutePunishBehavior(enemy, player, distanceX, distanceY);
        break;

    case EnhancedAIState::RECOVER:
        if (platforms)
        {
            ExecuteRecoverBehavior(enemy, *platforms, distanceX, distanceY);
        }
        break;

    case EnhancedAIState::RETREAT:
        ExecuteRetreatBehavior(enemy, player, distanceX, distanceY);
        break;

    case EnhancedAIState::EDGE_GUARD:
        ExecuteEdgeGuardBehavior(enemy, player, distanceX, distanceY);
        break;

    case EnhancedAIState::LEDGE_TRAP:
        ExecuteLedgeTrapBehavior(enemy, player, distanceX, distanceY);
        break;

    case EnhancedAIState::COMBO:
        // This needs the AI state to track combo progress
        // We'll handle this in the AIController
        break;
    }
}

void AIExecutor::ExecuteNeutralBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Neutral behavior: observe and position strategically

    // Move toward center stage if far from it
    float centerX = SCREEN_WIDTH / 2;
    if (enemy->physics.position.x < centerX - 50)
    {
        enemy->moveRight();
        enemy->stateManager.isFacingRight = true;
    }
    else if (enemy->physics.position.x > centerX + 50)
    {
        enemy->moveLeft();
        enemy->stateManager.isFacingRight = false;
    }

    // Face the player
    enemy->stateManager.isFacingRight = (player->physics.position.x > enemy->physics.position.x);

    // Occasionally shield preemptively
    if (GetRandomValue(0, 100) < 5 * config.difficulty.executionPrecision)
    {
        enemy->shield();
    }

    // Occasionally jump to platforms
    if (GetRandomValue(0, 100) < 3 * config.difficulty.executionPrecision &&
        enemy->stateManager.state != JUMPING)
    {
        enemy->jump();
    }
}

void AIExecutor::ExecuteApproachBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Smart approach with spacing awareness
    float absDistanceX = std::fabs(distanceX);
    float absDistanceY = std::fabs(distanceY);

    // Optimal spacing distance for approaching
    float optimalSpace = 70.0f;

    // If we're at optimal spacing, we might want to attack or shield
    if (absDistanceX < optimalSpace + 10 && absDistanceX > optimalSpace - 10)
    {
        // At optimal spacing, consider attacking or defending
        if (GetRandomValue(0, 100) < 30 * config.difficulty.executionPrecision)
        {
            ExecuteAttackBehavior(enemy, player, distanceX, distanceY);
            return;
        }
    }

    // Otherwise approach intelligently
    if (distanceX > optimalSpace)
    {
        enemy->moveRight();
        enemy->stateManager.isFacingRight = true;
    }
    else if (distanceX < -optimalSpace)
    {
        enemy->moveLeft();
        enemy->stateManager.isFacingRight = false;
    }

    // Jump to approach when needed
    if (distanceY < -80 && absDistanceX < 150 &&
        GetRandomValue(0, 100) > 70 && enemy->stateManager.isJumping)
    {
        enemy->jump();
    }

    // Dash dance near optimal spacing (advanced technique)
    if (absDistanceX < optimalSpace + 30 && absDistanceX > optimalSpace - 30)
    {
        if (GetRandomValue(0, 100) < 15 * config.difficulty.executionPrecision)
        {
            // Briefly dash in opposite direction
            if (distanceX > 0)
            {
                enemy->moveLeft();
                // Quick reversal to maintain facing
                if (GetRandomValue(0, 100) < 80)
                {
                    enemy->stateManager.isFacingRight = true;
                }
            }
            else
            {
                enemy->moveRight();
                if (GetRandomValue(0, 100) < 80)
                {
                    enemy->stateManager.isFacingRight = false;
                }
            }
        }
    }

    // Short hop aerials approach (advanced technique)
    if (absDistanceX < 130 && absDistanceY < 50 &&
        GetRandomValue(0, 100) > 80 && enemy->stateManager.isJumping)
    {
        enemy->jump();
    }

    // Fast fall when above target position to close vertical distance
    if (distanceY > 30 && enemy->physics.velocity.y > 0)
    {
        enemy->fastFall();
    }
}

void AIExecutor::ExecuteAttackBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Face the player
    enemy->stateManager.isFacingRight = (distanceX > 0);

    // Choose best attack based on situation
    int attackChoice = ChooseBestAttack(enemy, player, distanceX, distanceY);

    // Execute the chosen attack
    switch (attackChoice)
    {
    case JAB:
        enemy->jab();
        break;
    case FORWARD_TILT:
        enemy->forwardTilt();
        break;
    case UP_TILT:
        enemy->upTilt();
        break;
    case DOWN_TILT:
        enemy->downTilt();
        break;
    case DASH_ATTACK:
        enemy->dashAttack();
        break;
    case FORWARD_SMASH:
        enemy->forwardSmash(GetRandomValue(10, 25) * config.difficulty.executionPrecision);
        break;
    case UP_SMASH:
        enemy->upSmash(GetRandomValue(10, 25) * config.difficulty.executionPrecision);
        break;
    case DOWN_SMASH:
        enemy->downSmash(GetRandomValue(10, 25) * config.difficulty.executionPrecision);
        break;
    case NEUTRAL_AIR:
        enemy->neutralAir();
        break;
    case FORWARD_AIR:
        enemy->forwardAir();
        break;
    case BACK_AIR:
        enemy->backAir();
        break;
    case UP_AIR:
        enemy->upAir();
        break;
    case DOWN_AIR:
        enemy->downAir();
        break;
    case NEUTRAL_SPECIAL:
        enemy->neutralSpecial();
        break;
    case SIDE_SPECIAL:
        enemy->sideSpecial();
        break;
    case UP_SPECIAL:
        enemy->upSpecial();
        break;
    case DOWN_SPECIAL:
        enemy->downSpecial();
        break;
    case GRAB:
        enemy->grab();
        break;
    default:
        // If no specific attack was chosen, do a jab as fallback
        enemy->jab();
        break;
    }

    // Handle throws if grabbing
    if (enemy->stateManager.isGrabbing)
    {
        int throwChoice = GetRandomValue(0, 100);

        // Choose optimal throw based on position and damage
        if (player->damagePercent > 100 && enemy->physics.position.x < 200)
        {
            // Back throw for KO when near edge
            enemy->backThrow();
        }
        else if (player->damagePercent > 100 && enemy->physics.position.x > SCREEN_WIDTH - 200)
        {
            // Forward throw for KO when near edge
            enemy->forwardThrow();
        }
        else if (player->damagePercent < 50)
        {
            // Down throw for combos at low percent
            enemy->downThrow();
        }
        else
        {
            // Up throw at mid percent
            enemy->upThrow();
        }
    }
}

void AIExecutor::ExecutePressureBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Face the player
    enemy->stateManager.isFacingRight = (distanceX > 0);

    // Pressure involves staying close and using safe moves to maintain advantage
    float absDistanceX = std::fabs(distanceX);

    // Keep optimal pressure distance
    float pressureDistance = 50.0f;
    if (absDistanceX > pressureDistance + 20)
    {
        // Move toward player if too far
        if (distanceX > 0)
        {
            enemy->moveRight();
        }
        else
        {
            enemy->moveLeft();
        }
    }
    else if (absDistanceX < pressureDistance - 20)
    {
        // Back up slightly if too close
        if (distanceX > 0)
        {
            enemy->moveLeft();
            enemy->stateManager.isFacingRight = true; // Still face player
        }
        else
        {
            enemy->moveRight();
            enemy->stateManager.isFacingRight = false; // Still face player
        }
    }

    // Use safe, quick attacks to maintain pressure
    int frameCount = GetRandomValue(0, 100);
    if (frameCount % 20 == 0)
    {
        int attackChoice = GetRandomValue(0, 100);

        if (attackChoice < 30)
        {
            enemy->jab();
        }
        else if (attackChoice < 50)
        {
            enemy->forwardTilt();
        }
        else if (attackChoice < 70)
        {
            enemy->downTilt();
        }
        else if (attackChoice < 85 && enemy->stateManager.isJumping)
        {
            enemy->neutralAir();
        }
        else if (attackChoice < 95)
        {
            // Occasionally grab to mix up pressure
            enemy->grab();
        }
    }

    // Shield if player attacks
    if (player->stateManager.isAttacking && frameCount % 15 == 0)
    {
        enemy->shield();
    }
    else if (enemy->stateManager.isShielding && !player->stateManager.isAttacking)
    {
        enemy->releaseShield();
    }

    // Jump to follow player on platforms
    if (distanceY < -50 && frameCount % 30 == 0)
    {
        enemy->jump();
    }
}

void AIExecutor::ExecuteBaitBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Face the player
    enemy->stateManager.isFacingRight = (distanceX > 0);

    // Dash dance (move back and forth) to bait attacks
    int frameCount = GetRandomValue(0, 100);
    int dashFrame = frameCount % 20;

    if (dashFrame < 10)
    {
        // Dash in one direction
        if (distanceX > 0)
        {
            enemy->moveRight();
        }
        else
        {
            enemy->moveLeft();
        }
    }
    else
    {
        // Dash in opposite direction
        if (distanceX > 0)
        {
            enemy->moveLeft();
            enemy->stateManager.isFacingRight = true; // Maintain facing toward player
        }
        else
        {
            enemy->moveRight();
            enemy->stateManager.isFacingRight = false; // Maintain facing toward player
        }
    }

    // Empty short hops to bait anti-air responses
    if (frameCount % 45 == 0 && enemy->stateManager.isJumping)
    {
        enemy->jump();
        // Don't attack, just empty hop
    }

    // Shield briefly and then drop shield to bait grabs
    if (frameCount % 60 == 0)
    {
        enemy->shield();
    }
    else if (enemy->stateManager.isShielding && frameCount % 60 == 10)
    {
        enemy->releaseShield();
    }
}

void AIExecutor::ExecuteDefendBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Face the player
    enemy->stateManager.isFacingRight = (distanceX > 0);

    float absDistanceX = std::fabs(distanceX);
    float absDistanceY = std::fabs(distanceY);

    // If player is attacking, choose best defensive option
    if (player->stateManager.isAttacking)
    {
        int defenseChoice = GetRandomValue(0, 100);

        // Perfect shielding (precise timing)
        if (defenseChoice < 40 * config.difficulty.executionPrecision &&
            absDistanceX < 80 && absDistanceY < 60)
        {
            enemy->shield();
        }
        // Spotdodge (for close attacks, especially grabs)
        else if (defenseChoice < (40 + 20 * config.difficulty.executionPrecision) &&
            absDistanceX < 50 && absDistanceY < 40)
        {
            enemy->spotDodge();
        }
        // Roll away from danger
        else if (defenseChoice < (60 + 25 * config.difficulty.executionPrecision))
        {
            if (distanceX > 0)
            {
                enemy->backDodge(); // Roll away if player is to the right
            }
            else
            {
                enemy->forwardDodge(); // Roll away if player is to the left
            }
        }
        // Jump away (especially good for avoiding ground attacks)
        else if (enemy->stateManager.isJumping && enemy->stateManager.state != FALLING)
        {
            enemy->jump();

            // Air dodge if needed after jump
            if (GetRandomValue(0, 100) > 70)
            {
                float dodgeX = (distanceX > 0) ? -1.0f : 1.0f;
                float dodgeY = -0.5f;
                enemy->airDodge(dodgeX, dodgeY);
            }
        }
    }
    // If player is grabbing or has grabbed, escape
    else if (player->stateManager.isGrabbing)
    {
        // Button mashing to escape grabs faster
        int escapeAction = GetRandomValue(0, 3);
        switch (escapeAction)
        {
        case 0: enemy->moveLeft();
            break;
        case 1: enemy->moveRight();
            break;
        case 2: enemy->jump();
            break;
        case 3: enemy->shield();
            enemy->releaseShield();
            break;
        }
    }
    // If shielding and player isn't attacking anymore, release shield
    else if (enemy->stateManager.isShielding && !player->stateManager.isAttacking)
    {
        enemy->releaseShield();
    }
}

void AIExecutor::ExecutePunishBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Face the player
    enemy->stateManager.isFacingRight = (distanceX > 0);

    float absDistanceX = std::fabs(distanceX);
    float absDistanceY = std::fabs(distanceY);

    // For successful shield punish when player is in endlag
    if (player->stateManager.isAttacking && player->stateManager.attackFrame > player->stateManager.attackDuration *
        0.6f)
    {
        // Close range punishes
        if (absDistanceX < 50 && absDistanceY < 40)
        {
            // Grab punish
            if (GetRandomValue(0, 100) < 70 * config.difficulty.executionPrecision)
            {
                enemy->grab();
            }
            else
            {
                // Up smash out of shield
                enemy->upSmash(10 * config.difficulty.executionPrecision);
            }
        }
        // Mid range punishes
        else if (absDistanceX < 120 && absDistanceY < 60)
        {
            int punishOption = GetRandomValue(0, 100);
            if (punishOption < 40)
            {
                enemy->dashAttack();
            }
            else if (punishOption < 70)
            {
                enemy->forwardSmash(15 * config.difficulty.executionPrecision);
            }
            else
            {
                enemy->sideSpecial();
            }
        }
    }
    // If player is in hitstun, follow up with a combo starter
    else if (player->stateManager.isHitstun)
    {
        // Use a strong attack based on percent
        if (player->damagePercent < 50)
        {
            // Low percent - use combo starters
            if (GetRandomValue(0, 100) < 70)
            {
                enemy->upTilt();
            }
            else
            {
                enemy->grab();
            }
        }
        else if (player->damagePercent < 100)
        {
            // Mid percent - use launchers
            if (GetRandomValue(0, 100) < 60)
            {
                enemy->upAir();
            }
            else
            {
                enemy->forwardAir();
            }
        }
        else
        {
            // High percent - use KO moves
            if (GetRandomValue(0, 100) < 50)
            {
                enemy->forwardSmash(20 * config.difficulty.executionPrecision);
            }
            else
            {
                enemy->upSmash(20 * config.difficulty.executionPrecision);
            }
        }
    }
}

void AIExecutor::ExecuteRecoverBehavior(Character* enemy, const std::vector<Platform>& platforms, float distanceX,
                                        float distanceY)
{
    // Find the main platform
    Rectangle mainPlatform = platforms[0].rect; // Default to first platform
    float largestArea = mainPlatform.width * mainPlatform.height;

    for (size_t i = 1; i < platforms.size(); i++)
    {
        float area = platforms[i].rect.width * platforms[i].rect.height;
        if (area > largestArea)
        {
            mainPlatform = platforms[i].rect;
            largestArea = area;
        }
    }

    float absDistanceX = std::fabs(distanceX);

    // Calculate optimal recovery target point (aim for the center or nearest edge)
    float targetX;
    // Difficulty affects where we aim
    if (config.difficulty.recoverySkill < 0.4f) {
        // Low skill always aims for center - sometimes overshoots
        targetX = mainPlatform.x + mainPlatform.width / 2;
        
        // Random targeting errors for low skill
        if (GetRandomValue(0, 100) > 70) {
            targetX += GetRandomValue(-100, 100);
        }
    } else {
        // Higher skill intelligently picks closest point
        if (enemy->physics.position.x < mainPlatform.x) {
            targetX = mainPlatform.x + 50; // Aim just inside the left edge
        }
        else if (enemy->physics.position.x > mainPlatform.x + mainPlatform.width) {
            targetX = mainPlatform.x + mainPlatform.width - 50; // Aim just inside the right edge
        }
        else {
            targetX = mainPlatform.x + mainPlatform.width / 2;
        }
    }
    
    float targetY = mainPlatform.y - 20; // Just above platform

    // First check - if we're actually ON the main platform already, don't try to recover
    bool onMainPlatform = enemy->physics.position.x >= mainPlatform.x && 
                         enemy->physics.position.x <= mainPlatform.x + mainPlatform.width &&
                         std::abs(enemy->physics.position.y - mainPlatform.y) < 10;
                         
    if (onMainPlatform) {
        // We're already on the main platform, don't need recovery actions
        // Just make sure we're not moving off it
        if (enemy->physics.position.x < mainPlatform.x + 50) {
            enemy->moveRight();
        } else if (enemy->physics.position.x > mainPlatform.x + mainPlatform.width - 50) {
            enemy->moveLeft();
        }
        return;
    }

    // Calculate angle for recovery
    float recoveryAngle = CalculateRecoveryAngle(enemy, platforms);

    // Extreme danger - low recovery has higher priority
    bool dangerouslyLow = enemy->physics.position.y > BLAST_ZONE_BOTTOM - 200;
    bool dangerouslyHorizontal = enemy->physics.position.x < GameConfig::BLAST_ZONE_LEFT + 150 ||
                             enemy->physics.position.x > GameConfig::BLAST_ZONE_RIGHT - 150;

    // First priority: get horizontal alignment with stage
    if (enemy->physics.position.x < targetX - 50)
    {
        enemy->moveRight();
    }
    else if (enemy->physics.position.x > targetX + 50)
    {
        enemy->moveLeft();
    }

    // Low skill AI may make input errors based on executionPrecision
    if (config.difficulty.executionPrecision < 0.5f && GetRandomValue(0, 100) > 80) {
        // Sometimes moves in wrong direction
        if (GetRandomValue(0, 1) == 0) {
            enemy->moveLeft();
        } else {
            enemy->moveRight();
        }
    }

    // If below stage and has double jump, use it when appropriate
    if (enemy->physics.position.y > targetY && 
        enemy->stateManager.hasDoubleJump && !enemy->stateManager.isJumping)
    {
        // Double jump timing depends on skill
        bool shouldJump = false;
        
        // Low skill wastes double jump immediately
        if (config.difficulty.recoverySkill < 0.4f) {
            shouldJump = true;
        }
        // Medium skill jumps when in danger
        else if (config.difficulty.recoverySkill < 0.7f) {
            if (dangerouslyLow || dangerouslyHorizontal) {
                shouldJump = true;
            }
        }
        // High skill uses double jump strategically
        else {
            // Use double jump immediately if in serious danger
            if (dangerouslyLow || dangerouslyHorizontal) {
                shouldJump = true;
            }
            // Otherwise, be more strategic about jump timing
            else if (std::fabs(enemy->physics.position.x - targetX) < 200) {
                // We're close horizontally, safe to double jump
                shouldJump = true;
            }
        }
        
        // Input errors for low skill
        if (config.difficulty.executionPrecision < 0.3f && GetRandomValue(0, 100) > 70) {
            // Sometimes fails to input jump
            shouldJump = false;
        }
        
        if (shouldJump) {
            enemy->jump(); // This will use double jump if needed
        }
    }

    // Use up special for recovery, but save it for the right moment
    if (enemy->physics.position.y > targetY &&
        !enemy->stateManager.isJumping && !enemy->stateManager.hasDoubleJump &&
        enemy->stateManager.specialUpCD.current <= 0)
    {
        // Up special timing depends on skill
        bool shouldUpSpecial = false;
        
        // Low skill uses up special randomly or too early
        if (config.difficulty.recoverySkill < 0.4f) {
            shouldUpSpecial = (GetRandomValue(0, 100) > 50);
        }
        // Medium skill uses up special when in danger
        else if (config.difficulty.recoverySkill < 0.7f) {
            if (dangerouslyLow || dangerouslyHorizontal) {
                shouldUpSpecial = true;
            }
        }
        // High skill uses up special optimally
        else {
            // Use up special immediately if in serious danger
            if (dangerouslyLow || dangerouslyHorizontal) {
                shouldUpSpecial = true;
            }
            // Otherwise, be more strategic about up special timing
            else if (std::fabs(enemy->physics.position.x - targetX) < 150) {
                // We're pretty close horizontally, good time for up special
                shouldUpSpecial = true;
            }
        }
        
        // Input errors for low skill
        if (config.difficulty.executionPrecision < 0.3f && GetRandomValue(0, 100) > 70) {
            // Sometimes fails to input special
            shouldUpSpecial = false;
        }
        
        if (shouldUpSpecial) {
            enemy->upSpecial();
        }
    }

    // Air dodge as a recovery mixup or extension - only for higher skill AI
    if (enemy->physics.position.y > targetY && 
        std::fabs(enemy->physics.position.x - targetX) < 200 &&
        !enemy->stateManager.isJumping && !enemy->stateManager.hasDoubleJump &&
        enemy->stateManager.specialUpCD.current > 0 && !enemy->stateManager.isDodging)
    {
        // Only high skill AI uses air dodge effectively for recovery
        if (config.difficulty.techSkill > 0.5f && GetRandomValue(0, 100) > 50) {
            // Calculate best air dodge angle for recovery - more precise
            float dodgeX = (enemy->physics.position.x < targetX) ? 0.7f : -0.7f;
            // Adjust vertical component based on how far down we are
            float dodgeY = -0.7f;
            
            // If dangerously low, prioritize upward airdodge
            if (dangerouslyLow) {
                dodgeY = -0.9f;
                dodgeX *= 0.4f; // Reduce horizontal component
            }
            
            // Less skilled AI might use suboptimal angles
            if (config.difficulty.recoverySkill < 0.7f) {
                dodgeX += (GetRandomValue(-30, 30) / 100.0f);
                dodgeY += (GetRandomValue(-20, 20) / 100.0f);
            }
            
            enemy->airDodge(dodgeX, dodgeY);
        }
    }

    // If close to danger zone, prioritize getting back
    if (dangerouslyLow || dangerouslyHorizontal)
    {
        // Maximum effort to recover
        if (!enemy->stateManager.isJumping && enemy->stateManager.hasDoubleJump)
        {
            // Low skill might panic and miss inputs
            if (config.difficulty.executionPrecision < 0.3f && GetRandomValue(0, 100) > 60) {
                // Do nothing - panicking
            } else {
                enemy->jump();
            }
        }
        else if (enemy->stateManager.specialUpCD.current <= 0)
        {
            // Low skill might panic and miss inputs
            if (config.difficulty.executionPrecision < 0.3f && GetRandomValue(0, 100) > 60) {
                // Do nothing - panicking
            } else {
                enemy->upSpecial();
            }
        }
    }
}

void AIExecutor::ExecuteRetreatBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Default safe boundaries (fallback if platforms not set)
    float leftBoundary = 150;  // Safe distance from left edge
    float rightBoundary = GameConfig::SCREEN_WIDTH - 150;  // Safe distance from right edge
    
    // Get stage boundaries from platforms if available
    if (platforms && !platforms->empty()) {
        // Find the main platform (usually the largest one at the bottom)
        Rectangle mainPlatform = (*platforms)[0].rect;
        float largestArea = mainPlatform.width * mainPlatform.height;

        for (size_t i = 1; i < platforms->size(); i++) {
            float area = (*platforms)[i].rect.width * (*platforms)[i].rect.height;
            if (area > largestArea) {
                mainPlatform = (*platforms)[i].rect;
                largestArea = area;
            }
        }
        
        // Calculate safe retreat boundaries with margin
        leftBoundary = mainPlatform.x + 75;  // Increased margin for safety
        rightBoundary = mainPlatform.x + mainPlatform.width - 75;
    }
    
    // Check if we're at stage boundary
    bool atLeftBoundary = enemy->physics.position.x <= leftBoundary;
    bool atRightBoundary = enemy->physics.position.x >= rightBoundary;
    
    // Move away from player but keep facing them for defense
    // AND ensure we don't retreat off the stage
    if (distanceX > 0) {
        // Player is to our right, so retreat left
        if (!atLeftBoundary) {
            enemy->moveLeft();
            enemy->stateManager.isFacingRight = true; // Still face player while retreating
        } else {
            // At left boundary, stop retreating and either shield or counter-attack
            if (std::fabs(distanceX) < 150) {
                // Player is close, shield
                enemy->shield();
            } else {
                // Player is far enough, use projectile or prepare to counterattack
                if (GetRandomValue(0, 1) == 0) {
                    enemy->neutralSpecial();
                } else {
                    // Ready position, no action
                }
            }
        }
    } else {
        // Player is to our left, so retreat right
        if (!atRightBoundary) {
            enemy->moveRight();
            enemy->stateManager.isFacingRight = false; // Still face player while retreating
        } else {
            // At right boundary, stop retreating and either shield or counter-attack
            if (std::fabs(distanceX) < 150) {
                // Player is close, shield
                enemy->shield();
            } else {
                // Player is far enough, use projectile or prepare to counterattack
                if (GetRandomValue(0, 1) == 0) {
                    enemy->neutralSpecial();
                } else {
                    // Ready position, no action
                }
            }
        }
    }

    // Shield if player approaches too quickly or if we're cornered
    if ((std::fabs(distanceX) < 100 && player->physics.velocity.x != 0 &&
        ((distanceX > 0 && player->physics.velocity.x > 3) ||
         (distanceX < 0 && player->physics.velocity.x < -3))) ||
        (atLeftBoundary && distanceX < 0) || (atRightBoundary && distanceX > 0)) 
    {
        enemy->shield();
    }

    // Jump to platform to reset position, but only if not near edge and player is close
    bool nearEdge = atLeftBoundary || atRightBoundary || 
                   (enemy->physics.position.x < leftBoundary + 60) || 
                   (enemy->physics.position.x > rightBoundary - 60);
    
    if (!nearEdge && std::fabs(distanceX) < 150 && GetRandomValue(0, 100) > 80) // Greatly reduced frequency of jumps
    {
        enemy->jump();
    }

    // Use projectiles to keep player away, especially if at boundary
    bool shouldUseProjectile = (atLeftBoundary && distanceX < 0) || 
                              (atRightBoundary && distanceX > 0) ||
                              (std::fabs(distanceX) > 100 && GetRandomValue(0, 100) > 60);
                              
    if (shouldUseProjectile) 
    {
        enemy->neutralSpecial();
    }

    // Fast fall only when above a safe platform area
    bool aboveSafePlatform = enemy->physics.position.x >= leftBoundary + 50 && 
                            enemy->physics.position.x <= rightBoundary - 50;
                            
    if (enemy->physics.velocity.y > 0 && enemy->stateManager.state == FALLING && aboveSafePlatform)
    {
        enemy->fastFall();
    }
    
    // Spot dodge if player is very close and attacking
    if (std::fabs(distanceX) < 60 && player->stateManager.isAttacking) {
        enemy->spotDodge();
    }
    
    // If near edge and player is approaching, consider counterattack
    if ((atLeftBoundary || atRightBoundary) && 
        std::fabs(distanceX) < 120 && std::fabs(distanceX) > 50 &&
        GetRandomValue(0, 100) > 70)
    {
        // Choose a quick counterattack when cornered
        enemy->jab();
    }
}

void AIExecutor::ExecuteEdgeGuardBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Determine which edge the player is trying to recover to
    float edgeX = (player->physics.position.x < SCREEN_WIDTH / 2) ? SCREEN_WIDTH / 2 - 300 : SCREEN_WIDTH / 2 + 300;

    // Move toward the edge
    if (enemy->physics.position.x < edgeX - 50)
    {
        enemy->moveRight();
    }
    else if (enemy->physics.position.x > edgeX + 50)
    {
        enemy->moveLeft();
    }

    float absDistanceX = std::fabs(distanceX);
    float absDistanceY = std::fabs(distanceY);

    // Player is trying to recover from below
    if (player->physics.position.y > SCREEN_HEIGHT - 150)
    {
        // If player is far below, prepare for their recovery
        if (player->physics.position.y > SCREEN_HEIGHT)
        {
            // Wait at edge
            if (std::fabs(enemy->physics.position.x - edgeX) < 50)
            {
                // Occasionally charge a smash attack at edge
                if (GetRandomValue(0, 100) > 50)
                {
                    enemy->downSmash(GetRandomValue(10, 30) * config.difficulty.executionPrecision);
                }

                // Or prepare to intercept with an aerial
                if (GetRandomValue(0, 100) > 60)
                {
                    // Jump off stage to intercept
                    enemy->jump();
                }
            }
        }
        // Player is close enough to intercept
        else if (absDistanceX < 150 && absDistanceY < 150)
        {
            // Jump off stage for aggressive edge guard
            if (enemy->stateManager.isJumping &&
                enemy->stateManager.state != FALLING &&
                GetRandomValue(0, 100) > 40)
            {
                enemy->jump();
            }

            // Use appropriate aerial based on position
            if (enemy->stateManager.isJumping ||
                enemy->stateManager.state == FALLING)
            {
                if (distanceY > 0 && absDistanceX < 100)
                {
                    // Player is below, use down air (spike)
                    enemy->downAir();
                }
                else if (absDistanceY < 50)
                {
                    // Player is beside, use back/forward air
                    if ((distanceX > 0 && enemy->stateManager.isFacingRight) ||
                        (distanceX < 0 && !enemy->stateManager.isFacingRight))
                    {
                        enemy->forwardAir();
                    }
                    else
                    {
                        enemy->backAir();
                    }
                }
            }
        }
    }
    // Player is trying to recover from the side
    else if ((player->physics.position.x < SCREEN_WIDTH / 2 - 300 ||
            player->physics.position.x > SCREEN_WIDTH / 2 + 300) &&
        player->physics.position.y < SCREEN_HEIGHT - 100)
    {
        // If player is attempting side recovery
        if (std::fabs(player->physics.position.y - enemy->physics.position.y) < 100)
        {
            // Use projectiles or side special to intercept
            if (GetRandomValue(0, 100) > 60)
            {
                enemy->neutralSpecial();
            }
            else if (GetRandomValue(0, 100) > 70)
            {
                enemy->sideSpecial();
            }
        }
    }
}

void AIExecutor::ExecuteLedgeTrapBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Determine which ledge the player is at
    float ledgeX = (player->physics.position.x < SCREEN_WIDTH / 2) ? SCREEN_WIDTH / 2 - 300 : SCREEN_WIDTH / 2 + 300;

    // Optimal position for ledge trapping (slightly away from ledge)
    float optimalX = ledgeX + (ledgeX < SCREEN_WIDTH / 2 ? 80 : -80);

    // Move to optimal position
    if (enemy->physics.position.x < optimalX - 10)
    {
        enemy->moveRight();
    }
    else if (enemy->physics.position.x > optimalX + 10)
    {
        enemy->moveLeft();
    }

    // Always face toward the ledge
    enemy->stateManager.isFacingRight = (ledgeX > enemy->physics.position.x);

    // Dash dance near ledge to bait and react
    if (GetRandomValue(0, 100) < 30)
    {
        if (enemy->physics.position.x < optimalX)
        {
            enemy->moveRight();
        }
        else
        {
            enemy->moveLeft();
        }
    }

    // React based on player position
    if (std::fabs(enemy->physics.position.x - optimalX) < 30)
    {
        int option = GetRandomValue(0, 100);

        // Options to cover different ledge getup options
        if (option < 25)
        {
            // Cover neutral getup with jab or grab
            if (GetRandomValue(0, 100) < 60)
            {
                enemy->jab();
            }
            else
            {
                enemy->grab();
            }
        }
        else if (option < 50)
        {
            // Cover roll getup with down smash
            enemy->downSmash(20 * config.difficulty.executionPrecision);
        }
        else if (option < 75)
        {
            // Cover jump getup with up air
            if (enemy->stateManager.isJumping && enemy->stateManager.state != FALLING)
            {
                enemy->jump();
            }
            else
            {
                enemy->upAir();
            }
        }
        else
        {
            // Cover attack getup with shield
            enemy->shield();
        }
    }
}

int AIExecutor::ChooseBestAttack(Character* enemy, Character* player, float distanceX, float distanceY)
{
    // Define utility values for different attacks
    std::vector<std::pair<int, float>> attackUtilities;

    // Check each attack's utility in current situation
    for (const auto& attackOption : attackOptions)
    {
        if (attackOption->IsViable(distanceX, distanceY, enemy))
        {
            float utility = attackOption->GetUtility(distanceX, distanceY, enemy, player);
            attackUtilities.push_back({attackOption->GetAttackType(), utility});
        }
    }

    // Add fallback options if no viable attacks
    if (attackUtilities.empty())
    {
        // Default to jab if in range
        if (std::fabs(distanceX) < 80.0f && std::fabs(distanceY) < 40.0f)
        {
            return JAB;
        }
        // Default to neutral special if at a distance
        else
        {
            return NEUTRAL_SPECIAL;
        }
    }

    // Sort by utility (descending)
    std::sort(attackUtilities.begin(), attackUtilities.end(),
              [](const std::pair<int, float>& a, const std::pair<int, float>& b)
              {
                  return a.second > b.second;
              });

    // At lower difficulties, introduce randomness to attack selection
    if (config.difficulty.executionPrecision < 1.0f && attackUtilities.size() > 1)
    {
        // Chance to pick suboptimal attack increases as difficulty decreases
        if (GetRandomValue(0, 100) < (1.0f - config.difficulty.executionPrecision) * 40.0f)
        {
            // Pick randomly from top 3 options or all options if fewer than 3
            int randomIndex = GetRandomValue(0, std::min(2, (int)attackUtilities.size() - 1));
            return attackUtilities[randomIndex].first;
        }
    }

    // Return best attack
    return attackUtilities[0].first;
}

float AIExecutor::CalculateRecoveryAngle(Character* enemy, const std::vector<Platform>& platforms)
{
    // Find the closest edge of a platform
    float closestEdgeX = SCREEN_WIDTH / 2;
    float closestEdgeY = SCREEN_HEIGHT;
    float minDist = 999999.0f;

    for (const auto& platform : platforms)
    {
        // Check left edge
        float leftDist = sqrtf(powf(platform.rect.x - enemy->physics.position.x, 2) +
            powf(platform.rect.y - enemy->physics.position.y, 2));
        if (leftDist < minDist)
        {
            minDist = leftDist;
            closestEdgeX = platform.rect.x;
            closestEdgeY = platform.rect.y;
        }

        // Check right edge
        float rightDist = sqrtf(powf(platform.rect.x + platform.rect.width - enemy->physics.position.x, 2) +
            powf(platform.rect.y - enemy->physics.position.y, 2));
        if (rightDist < minDist)
        {
            minDist = rightDist;
            closestEdgeX = platform.rect.x + platform.rect.width;
            closestEdgeY = platform.rect.y;
        }
    }

    // Calculate angle to closest edge
    float dx = closestEdgeX - enemy->physics.position.x;
    float dy = closestEdgeY - enemy->physics.position.y;

    return atan2f(dy, dx);
}

void AIExecutor::ApplyDirectionalInfluence(Character* enemy)
{
    // Apply optimal DI based on knockback direction

    // When getting knocked horizontally, DI upward to survive longer
    if (std::fabs(enemy->physics.velocity.x) > std::fabs(enemy->physics.velocity.y))
    {
        if (enemy->physics.velocity.x > 0)
        {
            // Being knocked right
            enemy->physics.velocity.y -= 0.2f * config.difficulty.executionPrecision;
            enemy->physics.velocity.x -= 0.05f * config.difficulty.executionPrecision;
        }
        else
        {
            // Being knocked left
            enemy->physics.velocity.y -= 0.2f * config.difficulty.executionPrecision;
            enemy->physics.velocity.x += 0.05f * config.difficulty.executionPrecision;
        }
    }
    // When getting knocked vertically, DI horizontally to survive longer
    else if (std::fabs(enemy->physics.velocity.y) > std::fabs(enemy->physics.velocity.x))
    {
        if (enemy->physics.velocity.y < 0)
        {
            // Being knocked up
            if (enemy->physics.position.x < SCREEN_WIDTH / 2)
            {
                enemy->physics.velocity.x += 0.2f * config.difficulty.executionPrecision;
            }
            else
            {
                enemy->physics.velocity.x -= 0.2f * config.difficulty.executionPrecision;
            }
        }
    }

    // Wiggle inputs to escape grabs faster (if grabbed)
    if (enemy->stateManager.isHitstun && GetRandomValue(0, 100) < 80 * config.difficulty.executionPrecision)
    {
        // Simulate directional inputs to escape grab faster
        int escapeAction = GetRandomValue(0, 3);
        switch (escapeAction)
        {
        case 0: enemy->physics.velocity.x += 0.1f;
            break;
        case 1: enemy->physics.velocity.x -= 0.1f;
            break;
        case 2: enemy->physics.velocity.y -= 0.1f;
            break;
        case 3: enemy->physics.velocity.y += 0.1f;
            break;
        }
    }
}
