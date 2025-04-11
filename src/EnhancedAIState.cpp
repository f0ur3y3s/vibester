// EnhancedAIState.cpp
#include "EnhancedAIState.h"
#include "character/Character.h"
#include <algorithm>
#include <cmath>

EnhancedAIState::EnhancedAIState() {
    currentState = NEUTRAL;
    stateTimer = 0;
    decisionDelay = 3;
    reactionTime = 2;
    lastAttackFrame = 0;
    wasPlayerAttacking = false;
    threatLevel = 0.0f;
    isOffStage = false;
    playerIsOffStage = false;
    lastDistanceX = 0.0f;
    lastDistanceY = 0.0f;
    adaptiveTimer = 0;
    comboState = false;
    comboCounter = 0;

    // Initialize positional awareness
    nearLeftEdge = false;
    nearRightEdge = false;
    abovePlayer = false;
    belowPlayer = false;

    // Initialize history tracking
    lastPlayerAttacks.clear();
    playerPositionHistory.clear();
    playerStateHistory.clear();

    // Initialize adaptation variables
    playerAggressionLevel = 0.5f;
    playerDefenseLevel = 0.5f;
    playerRecoveryPattern = 0.0f;
    playerEdgeHabit = 0.0f;

    // Initialize player habit analysis
    playerFavorsGround = false;
    playerFavorsAerial = false;
    playerShieldsOften = false;
    playerRollsOften = false;
    playerJumpsOutOfCombos = false;

    // Initialize risk assessment
    currentRiskLevel = 0.3f;
    riskTolerance = 0.5f;
    expectedReward = 0.0f;

    // Initialize stage control
    centerControlImportance = 0.7f;

    // Initialize combo system
    currentCombo.sequence.clear();
    currentCombo.startingDamage = 0.0f;
    currentCombo.isFinisher = false;
    currentCombo.hitstunRemaining = 0;

    // Initialize match awareness
    stockAdvantage = 0.0f;
    damageAdvantage = 0.0f;
}

void EnhancedAIState::UpdateState(Character* enemy, Character* player, int frameCount) {
    // Update player history for pattern recognition
    // Track player attack history (limit to last 10)
    if (player->stateManager.isAttacking && player->stateManager.attackFrame == 0) {
        lastPlayerAttacks.push_front(static_cast<int>(player->stateManager.currentAttack));
        if (lastPlayerAttacks.size() > 10) {
            lastPlayerAttacks.pop_back();
        }
    }

    // Track player position history (every 10 frames, last 60 frames)
    if (frameCount % 10 == 0) {
        playerPositionHistory.push_front(std::make_pair(player->physics.position, frameCount));
        if (playerPositionHistory.size() > 6) {
            playerPositionHistory.pop_back();
        }
    }

    // Track player state history
    if (frameCount % 5 == 0 || player->stateManager.state != playerStateHistory.front()) {
        playerStateHistory.push_front(player->stateManager.state);
        if (playerStateHistory.size() > 20) {
            playerStateHistory.pop_back();
        }
    }

    // Increment attack counter if player just started an attack
    if (player->stateManager.isAttacking && player->stateManager.attackFrame == 0 && 
        player->stateManager.currentAttack != AttackType::NONE) {
        playerAttackFrequency[static_cast<int>(player->stateManager.currentAttack)]++;
    }

    // Update advantage metrics
    stockAdvantage = enemy->stocks - player->stocks;
    damageAdvantage = player->damagePercent - enemy->damagePercent;

    // Timer increments
    stateTimer++;
    adaptiveTimer++;
}

void EnhancedAIState::AnalyzePlayerPatterns() {
    // Analyze player's movement tendencies
    int groundStates = 0;
    int aerialStates = 0;
    int shieldStates = 0;
    int rollStates = 0;

    for (auto state : playerStateHistory) {
        if (state == IDLE || state == RUNNING) {
            groundStates++;
        } else if (state == JUMPING || state == FALLING) {
            aerialStates++;
        } else if (state == SHIELDING) {
            shieldStates++;
        } else if (state == DODGING) {
            rollStates++;
        }
    }

    // Update player tendency flags
    playerFavorsGround = groundStates > (playerStateHistory.size() * 0.6f);
    playerFavorsAerial = aerialStates > (playerStateHistory.size() * 0.5f);
    playerShieldsOften = shieldStates > (playerStateHistory.size() * 0.3f);
    playerRollsOften = rollStates > (playerStateHistory.size() * 0.25f);

    // Calculate player aggression level
    int totalAttacks = 0;
    for (auto& pair : playerAttackFrequency) {
        totalAttacks += pair.second;
    }

    // Adjust aggression level based on attack frequency and movement
    playerAggressionLevel = std::min(1.0f, (float)totalAttacks / 50.0f);
    if (playerFavorsAerial) {
        playerAggressionLevel += 0.2f;
    }
    playerAggressionLevel = std::min(1.0f, playerAggressionLevel);

    // Adjust defense level based on shield and roll usage
    playerDefenseLevel = (playerShieldsOften ? 0.7f : 0.3f) + (playerRollsOften ? 0.3f : 0.1f);
    playerDefenseLevel = std::min(1.0f, playerDefenseLevel);
}

bool EnhancedAIState::DetectPlayerHabit(const std::deque<CharacterState>& history,
                                      CharacterState state, float threshold) {
    if (history.size() < 5) return false;

    int count = 0;
    for (auto s : history) {
        if (s == state) count++;
    }

    return (float)count / history.size() >= threshold;
}
