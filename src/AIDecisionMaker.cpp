// AIDecisionMaker.cpp
#include "AIDecisionMaker.h"
#include "EnhancedAIState.h"
#include "character/Character.h"
#include "Platform.h"
#include "GameConfig.h"
#include "CharacterConfig.h"
#include <algorithm>
#include <cmath>
#include <random>

// Use enum directly without namespace
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

AIDecisionMaker::AIDecisionMaker(AIConfig& config) : config(config)
{
    // Zone strategies will be initialized when needed
    zoneStrategies.clear();
}

void AIDecisionMaker::DetermineNextAction(std::vector<Character*>& players,
                                          const std::vector<Platform>& platforms,
                                          IAIState& state)
{
    // Cast to EnhancedAIState for specific functionality
    EnhancedAIState& aiState = static_cast<EnhancedAIState&>(state);

    // Skip state transition if reaction delay hasn't elapsed
    // This simulates human reaction time
    int reactionDelay = static_cast<int>(config.difficulty.reactionTimeBase +
        (GetRandomValue(0, 100) / 100.0f) *
        config.difficulty.reactionTimeVariance *
        (1.0f - config.difficulty.decisionQuality));

    // Expert AI can react during combo/recover, others may not react at all
    // if in the middle of something (modeling panic/tunnel vision)
    if (config.difficulty.adaptability < 0.5f &&
        (aiState.GetCurrentState() == EnhancedAIState::ATTACK ||
            aiState.GetCurrentState() == EnhancedAIState::RETREAT ||
            aiState.GetCurrentState() == EnhancedAIState::PRESSURE))
    {
        // Low adaptability means we stick with current state longer
        reactionDelay += 15;
    }

    // Add buffer to recovery reactions for low skill (makes for easier edge guarding)
    if (config.difficulty.recoverySkill < 0.5f &&
        aiState.GetCurrentState() == EnhancedAIState::RECOVER)
    {
        reactionDelay += 10;
    }

    if (aiState.stateTimer < reactionDelay &&
        (config.difficulty.adaptability < 0.8f || // Low adaptability AI doesn't switch quickly
            (aiState.GetCurrentState() != EnhancedAIState::RECOVER &&
                aiState.GetCurrentState() != EnhancedAIState::COMBO)))
    {
        return;
    }

    Character* player = players[0];
    Character* enemy = players[1];

    float distanceX = player->physics.position.x - enemy->physics.position.x;
    float distanceY = player->physics.position.y - enemy->physics.position.y;
    float absDistanceX = std::fabs(distanceX);
    float absDistanceY = std::fabs(distanceY);

    // Update zone awareness for tactical positioning
    UpdateZoneAwareness(aiState, enemy, player, platforms);

    // Update threat level based on player's state and distance
    UpdateThreatLevel(aiState, player, absDistanceX, absDistanceY);

    // Store potential state transitions with their priority scores
    std::vector<std::pair<EnhancedAIState::State, float>> stateOptions;

    // ==== Calculate priority scores for each potential state ====

    // RECOVER - highest priority if off stage
    if (aiState.IsOffStage())
    {
        // Make recovery priority dependent on actual danger level
        float recoveryPriority = 10.0f;

        // Check if we're just slightly off stage or in serious danger
        Vector2 pos = enemy->physics.position;
        bool inExtremeDanger = pos.x < GameConfig::BLAST_ZONE_LEFT + 80 ||
            pos.x > GameConfig::BLAST_ZONE_RIGHT - 80 ||
            pos.y < GameConfig::BLAST_ZONE_TOP + 80 ||
            pos.y > GameConfig::BLAST_ZONE_BOTTOM - 80;

        if (!inExtremeDanger)
        {
            // If not in extreme danger, consider other actions too
            recoveryPriority = 8.5f;
        }

        stateOptions.push_back({EnhancedAIState::RECOVER, recoveryPriority});
    }

    // EDGE_GUARD - high priority if player is off stage and we're not
    if (aiState.IsPlayerOffStage() && !aiState.IsOffStage())
    {
        // Higher priority if player is at high damage
        float edgeGuardPriority = 7.0f + (player->damagePercent / 200.0f) * 2.0f;
        stateOptions.push_back({EnhancedAIState::EDGE_GUARD, edgeGuardPriority});
    }

    // LEDGE_TRAP - if player is at ledge but not fully off stage
    bool playerAtLedge = (player->physics.position.x < GameConfig::BLAST_ZONE_LEFT + 200 ||
            player->physics.position.x > GameConfig::BLAST_ZONE_RIGHT - 200) &&
        !aiState.IsPlayerOffStage();
    if (playerAtLedge && !aiState.IsOffStage())
    {
        stateOptions.push_back({EnhancedAIState::LEDGE_TRAP, 6.5f});
    }

    // COMBO - high priority if we can execute a combo
    if (player->stateManager.isHitstun && AttemptCombo(aiState, enemy, player))
    {
        stateOptions.push_back({EnhancedAIState::COMBO, 9.0f});
    }

    // DEFEND - priority based on threat level and player's attack state
    if (player->stateManager.isAttacking && absDistanceX < 120 && absDistanceY < 100)
    {
        float defendPriority = 5.0f + aiState.GetThreatLevel() * 4.0f;

        // Adjust based on frame advantage of player's attack
        if (player->stateManager.attackFrame > player->stateManager.attackDuration * 0.7f)
        {
            // Player is in endlag, less need to defend
            defendPriority *= 0.5f;
        }

        stateOptions.push_back({EnhancedAIState::DEFEND, defendPriority});
    }

    // PUNISH - if player is in endlag or missed an attack
    bool playerInEndlag = player->stateManager.isAttacking && player->stateManager.attackFrame > player->stateManager.
        attackDuration * 0.6f;
    if (playerInEndlag && absDistanceX < 150 && absDistanceY < 100)
    {
        float punishPriority = 8.0f;
        stateOptions.push_back({EnhancedAIState::PUNISH, punishPriority});
    }

    // ATTACK - priority based on position and damage
    if (absDistanceX < 80 && absDistanceY < 60)
    {
        float attackPriority = 6.0f;

        // Increase priority if player is at high damage (for KO potential)
        if (player->damagePercent > 100)
        {
            attackPriority += 2.0f;
        }

        // Reduce priority if player is shielding
        if (player->stateManager.isShielding)
        {
            attackPriority *= 0.5f;
        }

        stateOptions.push_back({EnhancedAIState::ATTACK, attackPriority});
    }

    // PRESSURE - maintain offensive advantage
    if (aiState.damageAdvantage > 30 && absDistanceX < 150)
    {
        float pressurePriority = 5.0f + (aiState.damageAdvantage / 200.0f) * 3.0f;
        stateOptions.push_back({EnhancedAIState::PRESSURE, pressurePriority});
    }

    // BAIT - if player tends to attack predictably or shield a lot
    if (aiState.playerAttackFrequency[player->stateManager.currentAttack] > 5 || aiState.playerShieldsOften)
    {
        float baitPriority = 4.0f;

        // Increase if player is aggressive
        if (aiState.playerAggressionLevel > 0.7f)
        {
            baitPriority += 1.5f;
        }

        stateOptions.push_back({EnhancedAIState::BAIT, baitPriority});
    }

    // RETREAT - priority based on damage and risk
    if (enemy->damagePercent > 100 || aiState.GetThreatLevel() > 0.8f)
    {
        float retreatPriority = 4.0f + enemy->damagePercent / 50.0f;

        // Higher priority if at stock disadvantage
        if (aiState.stockAdvantage < 0)
        {
            retreatPriority += 2.0f;
        }

        stateOptions.push_back({EnhancedAIState::RETREAT, retreatPriority});
    }

    // APPROACH - default option, priority based on stage position
    {
        float approachPriority = 3.0f;

        // Increase priority if center stage control is important
        bool playerHasCenter = abs(player->physics.position.x - GameConfig::SCREEN_WIDTH / 2) < abs(
            enemy->physics.position.x - GameConfig::SCREEN_WIDTH / 2);
        if (playerHasCenter && aiState.centerControlImportance > 0.5f)
        {
            approachPriority += 2.0f;
        }

        stateOptions.push_back({EnhancedAIState::APPROACH, approachPriority});
    }

    // NEUTRAL - when assessing the situation
    if (absDistanceX > 200 || (enemy->stateManager.state == IDLE && GetRandomValue(0, 100) < 10))
    {
        float neutralPriority = 2.0f;
        stateOptions.push_back({EnhancedAIState::NEUTRAL, neutralPriority});
    }

    // Add risk adjustment to all options
    for (auto& option : stateOptions)
    {
        float risk = AssessRisk(enemy, player, option.first);
        float reward = PredictReward(enemy, player, option.first);

        // Risk-reward calculation
        float riskTolerance = aiState.riskTolerance;

        // Adjust risk tolerance based on stock advantage
        if (aiState.stockAdvantage > 0)
        {
            // If ahead, be more conservative
            riskTolerance *= 0.8f;
        }
        else if (aiState.stockAdvantage < 0)
        {
            // If behind, be more aggressive
            riskTolerance *= 1.3f;
        }

        // Final adjustment to priority based on risk and reward
        option.second = option.second * (1.0f - (risk * (1.0f - riskTolerance))) * (0.5f + reward * 0.5f);
    }

    // Apply random adjustment based on difficulty
    if (config.difficulty.decisionQuality < 1.0f)
    {
        for (auto& option : stateOptions)
        {
            float randomAdjust = (1.0f - config.difficulty.decisionQuality) * 3.0f *
                ((float)GetRandomValue(-100, 100) / 100.0f);
            option.second += randomAdjust;
        }
    }

    // Find highest priority state
    EnhancedAIState::State newState = ChooseBestState(stateOptions);

    // Only change state if it's different from current state
    if (newState != aiState.GetCurrentState())
    {
        aiState.SetCurrentState(newState);
    }
}

EnhancedAIState::State AIDecisionMaker::ChooseBestState(
    std::vector<std::pair<EnhancedAIState::State, float>>& stateOptions)
{
    EnhancedAIState::State bestState = EnhancedAIState::NEUTRAL; // Default
    float highestPriority = 0.0f;

    for (const auto& option : stateOptions)
    {
        if (option.second > highestPriority)
        {
            highestPriority = option.second;
            bestState = option.first;
        }
    }

    return bestState;
}

float AIDecisionMaker::AssessRisk(Character* enemy, Character* player, int stateId)
{
    EnhancedAIState::State potentialState = static_cast<EnhancedAIState::State>(stateId);
    float risk = 0.0f;

    // Base risk factors
    switch (potentialState)
    {
    case EnhancedAIState::EDGE_GUARD:
        // Risky if player has good recovery or we're at high damage
        risk = 0.6f + (enemy->damagePercent / 200.0f) * 0.3f;
        break;

    case EnhancedAIState::ATTACK:
        // Risk based on player's shield and reaction patterns
        risk = 0.4f;
        if (player->stateManager.isShielding) risk += 0.3f;
        break;

    case EnhancedAIState::COMBO:
        // Risky if player has good combo-breaking habits
        risk = 0.3f;
        break;

    case EnhancedAIState::RECOVER:
        // Very risky, especially at high damage percentages
        risk = 0.7f + (enemy->damagePercent / 150.0f) * 0.3f;
        break;

    case EnhancedAIState::PRESSURE:
        // Moderate risk, can be countered
        risk = 0.5f;
        break;

    case EnhancedAIState::RETREAT:
        // Low risk, but can cede stage control
        risk = 0.2f;
        break;

    case EnhancedAIState::DEFEND:
        // Low risk but can be baited
        risk = 0.3f;
        break;

    case EnhancedAIState::BAIT:
        // Moderate risk, depends on execution
        risk = 0.4f;
        break;

    case EnhancedAIState::NEUTRAL:
    case EnhancedAIState::APPROACH:
    default:
        // Low risk
        risk = 0.2f;
        break;
    }

    // Adjust risk based on player state
    if (player->stateManager.isAttacking)
    {
        // Higher risk if player is in active attack frames
        int activeStart = player->stateManager.attackDuration * 0.2f;
        int activeEnd = player->stateManager.attackDuration * 0.6f;

        if (player->stateManager.attackFrame >= activeStart && player->stateManager.attackFrame <= activeEnd)
        {
            risk += 0.2f;
        }
    }

    // Clamp risk value
    return std::min(1.0f, std::max(0.0f, risk));
}

float AIDecisionMaker::PredictReward(Character* enemy, Character* player, int stateId)
{
    EnhancedAIState::State potentialState = static_cast<EnhancedAIState::State>(stateId);
    float reward = 0.5f; // Default moderate reward

    switch (potentialState)
    {
    case EnhancedAIState::ATTACK:
        // Higher reward at high damage (KO potential)
        reward = 0.6f + (player->damagePercent / 150.0f) * 0.4f;
        break;

    case EnhancedAIState::EDGE_GUARD:
        // Very high reward if successful
        reward = 0.8f + (player->damagePercent / 200.0f) * 0.2f;
        break;

    case EnhancedAIState::COMBO:
        // High reward, especially at lower damage percentages
        reward = 0.7f + ((100.0f - std::min(100.0f, player->damagePercent)) / 100.0f) * 0.3f;
        break;

    case EnhancedAIState::RECOVER:
        // Necessary but not rewarding itself
        reward = 0.4f;
        break;

    case EnhancedAIState::PRESSURE:
        // Good for building damage
        reward = 0.6f;
        break;

    case EnhancedAIState::BAIT:
        // Potentially high reward if player is predictable
        reward = 0.5f;
        break;

    case EnhancedAIState::DEFEND:
        // Defensive reward depends on threat level
        reward = 0.3f;
        break;

    case EnhancedAIState::RETREAT:
        // Lower immediate reward but preserves stock
        reward = 0.3f + (enemy->damagePercent / 150.0f) * 0.4f;
        break;

    case EnhancedAIState::NEUTRAL:
        // Moderate reward from information gathering
        reward = 0.4f;
        break;

    case EnhancedAIState::APPROACH:
        // Moderate reward from gaining position
        reward = 0.5f;
        break;

    default:
        reward = 0.4f;
        break;
    }

    return std::min(1.0f, std::max(0.0f, reward));
}

void AIDecisionMaker::UpdateThreatLevel(EnhancedAIState& aiState, Character* player, float absDistanceX,
                                        float absDistanceY)
{
    // Base threat level depends on distance
    float distanceThreat = 1.0f - (std::min(absDistanceX, 500.0f) / 500.0f);

    // Attack threat - analyze based on attack type and frame
    float attackThreat = 0.0f;
    if (player->stateManager.isAttacking)
    {
        // Different attacks have different threat levels
        switch (player->stateManager.currentAttack)
        {
        case AttackType::FORWARD_SMASH:
        case AttackType::UP_SMASH:
        case AttackType::DOWN_SMASH:
            attackThreat = 0.8f; // Smash attacks are very threatening
            break;

        case AttackType::FORWARD_AIR:
        case AttackType::BACK_AIR:
        case AttackType::UP_AIR:
        case AttackType::DOWN_AIR:
            attackThreat = 0.6f; // Aerials are moderately threatening
            break;

        case AttackType::NEUTRAL_SPECIAL:
        case AttackType::SIDE_SPECIAL:
        case AttackType::UP_SPECIAL:
        case AttackType::DOWN_SPECIAL:
            attackThreat = 0.7f; // Specials can be dangerous
            break;

        case AttackType::GRAB:
            attackThreat = 0.75f; // Grabs are threatening, especially at high damage
            break;

        default:
            attackThreat = 0.5f; // Standard threat for other attacks
        }

        // Adjust based on attack frame - peak threat is during active frames
        int attackDuration = player->stateManager.attackDuration;
        int activeFramesStart = attackDuration * 0.2f; // Approximate start of active frames
        int activeFramesEnd = attackDuration * 0.6f; // Approximate end of active frames

        if (player->stateManager.attackFrame < activeFramesStart)
        {
            // Startup frames - increasing threat
            attackThreat *= (float)player->stateManager.attackFrame / activeFramesStart;
        }
        else if (player->stateManager.attackFrame > activeFramesEnd)
        {
            // Endlag frames - decreasing threat
            attackThreat *= 1.0f - ((float)(player->stateManager.attackFrame - activeFramesEnd) / (attackDuration -
                activeFramesEnd));
        }
    }

    // Damage threat - higher at critical percentages
    float damageThreat = std::min(1.0f, player->damagePercent / 120.0f);

    // Position threat - being cornered is dangerous
    float positionThreat = 0.0f;
    if (aiState.nearLeftEdge || aiState.nearRightEdge)
    {
        positionThreat = 0.3f;
    }

    // Combine threat factors with weighted importance
    float threatLevel = (distanceThreat * 0.3f) +
        (attackThreat * 0.4f) +
        (damageThreat * 0.2f) +
        (positionThreat * 0.1f);

    // Add random noise based on difficulty (lower difficulty = more inconsistent threat assessment)
    float randomFactor = (1.0f - config.difficulty.decisionQuality) * 0.2f * ((float)GetRandomValue(-100, 100) /
        100.0f);
    threatLevel = std::min(1.0f, std::max(0.0f, threatLevel + randomFactor));

    // Update the state
    aiState.threatLevel = threatLevel;
}

void AIDecisionMaker::UpdateZoneAwareness(EnhancedAIState& aiState, Character* enemy, Character* player,
                                          const std::vector<Platform>& platforms)
{
    // Main stage boundaries (approximate)
    float leftEdge = GameConfig::BLAST_ZONE_LEFT + 150;
    float rightEdge = GameConfig::BLAST_ZONE_RIGHT - 150;
    float stageWidth = rightEdge - leftEdge;

    // Update edge proximity
    aiState.nearLeftEdge = (enemy->physics.position.x < leftEdge + stageWidth * 0.2f);
    aiState.nearRightEdge = (enemy->physics.position.x > rightEdge - stageWidth * 0.2f);

    // Update vertical positioning
    aiState.abovePlayer = (enemy->physics.position.y < player->physics.position.y - 30);
    aiState.belowPlayer = (enemy->physics.position.y > player->physics.position.y + 30);

    // Define strategic zones if not already defined
    if (zoneStrategies.empty())
    {
        // Center stage zone
        ZoneStrategy centerZone;
        centerZone.zone = {
            leftEdge + stageWidth * 0.3f,
            BLAST_ZONE_TOP + 200,
            stageWidth * 0.4f,
            300
        };
        centerZone.preferredState = EnhancedAIState::NEUTRAL;
        centerZone.preferredAttacks = {
            AttackType::JAB,
            AttackType::FORWARD_TILT,
            AttackType::UP_TILT,
            AttackType::DOWN_TILT
        };
        centerZone.priorityMultiplier = 1.2f;
        zoneStrategies.push_back(centerZone);

        // Left edge zone
        ZoneStrategy leftEdgeZone;
        leftEdgeZone.zone = {
            leftEdge,
            BLAST_ZONE_TOP + 200,
            stageWidth * 0.2f,
            300
        };
        leftEdgeZone.preferredState = EnhancedAIState::EDGE_GUARD;
        leftEdgeZone.preferredAttacks = {
            AttackType::FORWARD_SMASH,
            AttackType::DOWN_SMASH,
            AttackType::BACK_AIR
        };
        leftEdgeZone.priorityMultiplier = 1.0f;
        zoneStrategies.push_back(leftEdgeZone);

        // Right edge zone
        ZoneStrategy rightEdgeZone;
        rightEdgeZone.zone = {
            rightEdge - stageWidth * 0.2f,
            BLAST_ZONE_TOP + 200,
            stageWidth * 0.2f,
            300
        };
        rightEdgeZone.preferredState = EnhancedAIState::EDGE_GUARD;
        rightEdgeZone.preferredAttacks = {
            AttackType::FORWARD_SMASH,
            AttackType::DOWN_SMASH,
            AttackType::BACK_AIR
        };
        rightEdgeZone.priorityMultiplier = 1.0f;
        zoneStrategies.push_back(rightEdgeZone);
    }
}

bool AIDecisionMaker::IsOffStage(Vector2 position, const std::vector<Platform>& platforms)
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

bool AIDecisionMaker::AttemptCombo(EnhancedAIState& aiState, Character* enemy, Character* player)
{
    // Check if we can start or continue a combo
    if (!player->stateManager.isHitstun) return false;

    // If combo database is empty, build it
    if (aiState.knownCombos.empty())
    {
        BuildComboDatabase(aiState);
    }

    // Find appropriate combo based on player's damage
    float playerDamage = player->damagePercent;

    for (const auto& combo : aiState.knownCombos)
    {
        if (playerDamage >= combo.startingDamage &&
            playerDamage < combo.startingDamage + 40)
        {
            aiState.currentCombo = combo;
            aiState.comboCounter = 0;
            return true;
        }
    }

    return false;
}

void AIDecisionMaker::BuildComboDatabase(EnhancedAIState& aiState)
{
    // Create starter combo for low percent
    EnhancedAIState::ComboData lowCombo;
    lowCombo.sequence = {AttackType::UP_TILT, AttackType::UP_TILT, AttackType::UP_AIR};
    lowCombo.startingDamage = 0;
    lowCombo.isFinisher = false;
    lowCombo.hitstunRemaining = 0;
    aiState.knownCombos.push_back(lowCombo);

    // Create mid percent combo
    EnhancedAIState::ComboData midCombo;
    midCombo.sequence = {AttackType::DOWN_TILT, AttackType::FORWARD_AIR};
    midCombo.startingDamage = 40;
    midCombo.isFinisher = false;
    midCombo.hitstunRemaining = 0;
    aiState.knownCombos.push_back(midCombo);

    // Create kill combo for high percent
    EnhancedAIState::ComboData killCombo;
    killCombo.sequence = {AttackType::DOWN_THROW, AttackType::UP_AIR, AttackType::UP_SPECIAL};
    killCombo.startingDamage = 90;
    killCombo.isFinisher = true;
    killCombo.hitstunRemaining = 0;
    aiState.knownCombos.push_back(killCombo);

    // Edge guarding combo
    EnhancedAIState::ComboData edgeCombo;
    edgeCombo.sequence = {AttackType::BACK_AIR, AttackType::DOWN_AIR};
    edgeCombo.startingDamage = 60;
    edgeCombo.isFinisher = true;
    edgeCombo.hitstunRemaining = 0;
    aiState.knownCombos.push_back(edgeCombo);
}
