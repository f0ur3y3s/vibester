// EnhancedAIController.cpp
#include "EnhancedAIController.h"
#include "character/Character.h"
#include "Platform.h"
#include "GameConfig.h"
#include "CharacterConfig.h"
#include <algorithm>
#include <cmath>

// Use the AttackType enum directly
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

EnhancedAIController::EnhancedAIController()
    : frameCount(0),
      wasComboEffective(false),
      shouldFeint(false),
      lastDIEffectiveness(0.5f)
{
    // Initialize with default config
    config = AIConfig(0.8f); // Default to challenging

    // Create component instances
    aiState = std::make_unique<EnhancedAIState>();
    decisionMaker = std::make_unique<AIDecisionMaker>(config);
    executor = std::make_unique<AIExecutor>(config);
}

void EnhancedAIController::Update(std::vector<Character*>& players, std::vector<Platform>& platforms)
{
    // Make sure we have at least two players
    if (players.size() < 2) return;

    Character* player = players[0];
    Character* enemy = players[1];

    // Skip AI update if the enemy is dead or dying
    if (enemy->stocks <= 0 || enemy->stateManager.isDying) return;

    // Increment frame counter
    frameCount++;

    // Apply directional influence if in hitstun
    if (enemy->stateManager.isHitstun && enemy->stateManager.hitstunFrames > 5)
    {
        executor->ApplyDirectionalInfluence(enemy);
        return;
    }

    // Get positions and calculate distances
    Vector2 playerPos = player->physics.position;
    Vector2 enemyPos = enemy->physics.position;
    float distanceX = playerPos.x - enemyPos.x;
    float distanceY = playerPos.y - enemyPos.y;
    float absDistanceX = std::fabs(distanceX);
    float absDistanceY = std::fabs(distanceY);

    // Check if player or AI is off stage
    bool enemyOffStage = IsOffStage(enemyPos, platforms);
    bool playerOffStage = IsOffStage(playerPos, platforms);
    aiState->SetOffStageStatus(enemyOffStage, playerOffStage);

    // Update AI state with current game state
    aiState->UpdateState(enemy, player, frameCount);

    // Analyze player patterns every 60 frames
    if (frameCount % 60 == 0)
    {
        aiState->AnalyzePlayerPatterns();
    }

    // Update executor with platform reference for recovery
    executor->SetPlatforms(&platforms);

    // Determine the best AI state based on current situation
    decisionMaker->DetermineNextAction(players, platforms, *aiState);

    // Special case for combo behavior which needs to track state
    if (aiState->GetCurrentState() == EnhancedAIState::COMBO)
    {
        ExecuteComboBehavior(enemy, player, distanceX, distanceY);
    }
    else
    {
        // Execute behavior based on current state
        executor->ExecuteAction(enemy, player, distanceX, distanceY, aiState->GetCurrentState());
    }
}

void EnhancedAIController::SetDifficulty(float difficulty)
{
    // Clamp difficulty to valid range
    config.SetDifficulty(std::max(0.0f, std::min(1.0f, difficulty)));

    // Update reaction times based on difficulty
    int reactionDelay = static_cast<int>(15.0f - (config.difficulty.decisionQuality * 10.0f));
    aiState->decisionDelay = reactionDelay;

    // Update risk tolerance
    aiState->riskTolerance = 0.3f + (config.difficulty.decisionQuality * 0.5f);
}

float EnhancedAIController::GetDifficulty() const
{
    return config.difficulty.decisionQuality;
}

EnhancedAIState::State EnhancedAIController::GetCurrentState() const
{
    return aiState->GetCurrentState();
}

float EnhancedAIController::GetCurrentConfidence() const
{
    return aiState->GetExpectedReward();
}

void EnhancedAIController::ExecuteComboBehavior(Character* enemy, Character* player, float distanceX, float distanceY)
{
    float absDistanceX = std::fabs(distanceX);
    float absDistanceY = std::fabs(distanceY);

    // If player is no longer in hitstun, combo is dropped
    if (!player->stateManager.isHitstun && aiState->stateTimer > 5)
    {
        aiState->SetCurrentState(EnhancedAIState::NEUTRAL);
        aiState->comboCounter = 0;
        return;
    }

    // Execute current combo if we have one
    if (!aiState->currentCombo.sequence.empty())
    {
        // Get next move in sequence
        int comboStep = aiState->comboCounter % aiState->currentCombo.sequence.size();
        int nextMove = aiState->currentCombo.sequence[comboStep];

        // Position for the next combo move
        float optimalDistX = 0.0f;
        float optimalDistY = 0.0f;

        // Set optimal positioning based on next move
        switch (nextMove)
        {
        case UP_TILT:
        case UP_SMASH:
            optimalDistX = 0.0f;
            optimalDistY = 10.0f;
            break;

        case FORWARD_AIR:
            optimalDistX = enemy->stateManager.isFacingRight ? 40.0f : -40.0f;
            optimalDistY = -20.0f;
            break;

        case UP_AIR:
            optimalDistX = 0.0f;
            optimalDistY = -40.0f;
            break;

        case BACK_AIR:
            optimalDistX = enemy->stateManager.isFacingRight ? -40.0f : 40.0f;
            optimalDistY = -10.0f;
            break;

        default:
            optimalDistX = 30.0f * (enemy->stateManager.isFacingRight ? 1.0f : -1.0f);
            optimalDistY = 0.0f;
            break;
        }

        // Move to optimal position
        if (distanceX < optimalDistX - 10)
        {
            enemy->moveRight();
            enemy->stateManager.isFacingRight = true;
        }
        else if (distanceX > optimalDistX + 10)
        {
            enemy->moveLeft();
            enemy->stateManager.isFacingRight = false;
        }

        // Jump or fast-fall to get vertical positioning
        if (distanceY < optimalDistY - 10 && enemy->stateManager.state != JUMPING)
        {
            enemy->jump();
        }
        else if (distanceY > optimalDistY + 10 && enemy->physics.velocity.y > 0)
        {
            enemy->fastFall();
        }

        // Execute the combo move when in position and timing is right
        // Timing depends on hitstun and positioning
        bool inPosition = std::fabs(distanceX - optimalDistX) < 20 && std::fabs(distanceY - optimalDistY) < 20;
        bool correctState = true;

        // Check if we're in the right state for the attack
        if ((nextMove >= NEUTRAL_AIR && nextMove <= DOWN_AIR) &&
            enemy->stateManager.state != JUMPING && enemy->stateManager.state != FALLING)
        {
            correctState = false;
        }

        // Execute attack if conditions are met
        if (inPosition && correctState && aiState->stateTimer % 10 == 0)
        {
            switch (nextMove)
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
                enemy->forwardSmash(10 * config.difficulty.executionPrecision);
                break;
            case UP_SMASH:
                enemy->upSmash(10 * config.difficulty.executionPrecision);
                break;
            case DOWN_SMASH:
                enemy->downSmash(10 * config.difficulty.executionPrecision);
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
            default:
                break;
            }

            // Increment combo counter
            aiState->comboCounter++;

            // If we've completed the combo, reset state
            if (aiState->comboCounter >= aiState->currentCombo.sequence.size())
            {
                if (aiState->currentCombo.isFinisher)
                {
                    // After finisher, go to neutral
                    aiState->SetCurrentState(EnhancedAIState::NEUTRAL);
                }
                else
                {
                    // After non-finisher, continue pressure
                    aiState->SetCurrentState(EnhancedAIState::PRESSURE);
                }
                aiState->comboCounter = 0;
            }
        }
    }

    // If combo state lasts too long, reset
    if (aiState->stateTimer > 120)
    {
        aiState->SetCurrentState(EnhancedAIState::NEUTRAL);
        aiState->comboCounter = 0;
    }
}

bool EnhancedAIController::IsOffStage(Vector2 position, const std::vector<Platform>& platforms)
{
    // Find the main platform (usually the largest one at the bottom)
    Rectangle mainPlatform = platforms[0].rect;
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

    // Check if position is not above main platform
    bool aboveMainStage = (position.x >= mainPlatform.x - 50 &&
        position.x <= mainPlatform.x + mainPlatform.width + 50 &&
        position.y < mainPlatform.y);

    // Check if position is beyond blastzones with a margin
    bool nearBlastzone = (position.x < GameConfig::BLAST_ZONE_LEFT + 100 ||
        position.x > GameConfig::BLAST_ZONE_RIGHT - 100 ||
        position.y < GameConfig::BLAST_ZONE_TOP + 100 ||
        position.y > GameConfig::BLAST_ZONE_BOTTOM - 100);

    // Fix: Only consider a character off-stage if they're not above the platform AND are far enough away horizontally
    // This prevents the AI from thinking it's off-stage when it's just in the air
    bool significantlyOffStage = !aboveMainStage &&
    (position.x < mainPlatform.x - 75 ||
        position.x > mainPlatform.x + mainPlatform.width + 75);

    // Consider danger zone near blast zone
    bool inDangerZone = position.x < GameConfig::BLAST_ZONE_LEFT + 60 ||
        position.x > GameConfig::BLAST_ZONE_RIGHT - 60 ||
        position.y < GameConfig::BLAST_ZONE_TOP + 60 ||
        position.y > GameConfig::BLAST_ZONE_BOTTOM - 60;

    // Only consider off stage if significantly off stage OR in real danger near blastzones
    return significantlyOffStage || inDangerZone;
}
