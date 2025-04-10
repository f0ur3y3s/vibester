#include "EnhancedEnemyAI.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <unordered_map>

// ================ EnhancedAIState Implementation ================

EnhancedAIState::EnhancedAIState() {
    currentState = NEUTRAL;
    stateTimer = 0;
    decisionDelay = 3;  // Master AI makes decisions even faster
    reactionTime = 2;   // Excellent reaction time
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
    for (int i = 0; i <= Character::DOWN_THROW; i++) {
        playerAttackFrequency[i] = 0;
    }
    
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

void EnhancedAIState::updateHistory(Character* player, int frameCount) {
    // Track player attack history (limit to last 10)
    if (player->isAttacking && player->attackFrame == 0) {
        lastPlayerAttacks.push_front(player->currentAttack);
        if (lastPlayerAttacks.size() > 10) {
            lastPlayerAttacks.pop_back();
        }
    }
    
    // Track player position history (every 10 frames, last 60 frames)
    if (frameCount % 10 == 0) {
        playerPositionHistory.push_front(std::make_pair(player->position, frameCount));
        if (playerPositionHistory.size() > 6) {
            playerPositionHistory.pop_back();
        }
    }
    
    // Track player state history
    if (frameCount % 5 == 0 || player->state != playerStateHistory.front()) {
        playerStateHistory.push_front(player->state);
        if (playerStateHistory.size() > 20) {
            playerStateHistory.pop_back();
        }
    }
    
    // Increment attack counter if player just started an attack
    if (player->isAttacking && player->attackFrame == 0 && player->currentAttack != Character::NONE) {
        playerAttackFrequency[player->currentAttack]++;
    }
}

void EnhancedAIState::analyzePlayerPatterns() {
    // Analyze player's movement tendencies
    int groundStates = 0;
    int aerialStates = 0;
    int shieldStates = 0;
    int rollStates = 0;
    
    for (auto state : playerStateHistory) {
        if (state == Character::IDLE || state == Character::RUNNING) {
            groundStates++;
        } else if (state == Character::JUMPING || state == Character::FALLING) {
            aerialStates++;
        } else if (state == Character::SHIELDING) {
            shieldStates++;
        } else if (state == Character::DODGING) {
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
    for (int i = 0; i <= Character::DOWN_THROW; i++) {
        totalAttacks += playerAttackFrequency[i];
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

bool EnhancedAIState::detectPlayerHabit(const std::deque<Character::CharacterState>& history, Character::CharacterState state, float threshold) {
    if (history.size() < 5) return false;
    
    int count = 0;
    for (auto s : history) {
        if (s == state) count++;
    }
    
    return (float)count / history.size() >= threshold;
}

// ================ JabAttack Implementation ================

float JabAttack::GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) {
    // Base utility for jab
    float utility = 0.5f;
    
    // Distance factors
    float optimalDistance = 60.0f;
    float distanceFactor = 1.0f - std::min(1.0f, std::fabs(std::fabs(distanceX) - optimalDistance) / 50.0f);
    utility *= distanceFactor;
    
    // Vertical position factor
    if (std::fabs(distanceY) > 40.0f) {
        utility *= 0.5f; // Jab is less effective if there's vertical separation
    }
    
    // Better when player is at low damage (for combo starters)
    if (player->damagePercent < 45.0f) {
        utility *= 1.2f;
    }
    
    // Better if player is grounded and facing us
    if (player->state != Character::JUMPING && player->state != Character::FALLING) {
        utility *= 1.3f;
    }
    
    // Less useful when player is shielding
    if (player->isShielding) {
        utility *= 0.3f;
    }
    
    return std::min(1.0f, utility);
}

void JabAttack::Execute(Character* enemy) {
    enemy->jab();
}

bool JabAttack::IsViable(float distanceX, float distanceY, Character* enemy) {
    // Check if enemy is in an appropriate state to jab
    if (enemy->state == Character::JUMPING || enemy->state == Character::FALLING) {
        return false;
    }
    
    // Check if within jab range
    return std::fabs(distanceX) < 80.0f && std::fabs(distanceY) < 40.0f;
}

// ================ EnhancedEnemyAI Implementation ================

EnhancedEnemyAI::EnhancedEnemyAI() {
    difficulty = 0.8f; // Default to hard
    frameCount = 0;
    lastDIEffectiveness = 0.5f;
    wasComboEffective = false;
    shouldFeint = false;
    
    // Initialize difficulty parameters
    reactionTimeBase = 5.0f;
    reactionTimeVariance = 5.0f;
    decisionQuality = 0.8f;
    executionPrecision = 0.9f;
    
    // Initialize attack options
    initializeAttackOptions();
    
    // Build the combo database
    BuildComboDatabase();
}

void EnhancedEnemyAI::initializeAttackOptions() {
    attackOptions.push_back(std::unique_ptr<AttackOption>(new JabAttack()));
    // Add other attack options here
    // attackOptions.push_back(std::make_unique<ForwardTiltAttack>());
    // attackOptions.push_back(std::make_unique<UpTiltAttack>());
    // etc.
}

// Get current AI state
EnhancedAIState::State EnhancedEnemyAI::GetCurrentState() const {
    return aiState.currentState;
}

// Get confidence level in current strategy
float EnhancedEnemyAI::GetCurrentConfidence() const {
    return aiState.expectedReward;
}

// Set difficulty level
void EnhancedEnemyAI::SetDifficulty(float diff) {
    difficulty = std::max(0.0f, std::min(1.0f, diff));
    AdaptToDifficulty();
}

// Helper function to check if a position is off the main stage
bool EnhancedEnemyAI::IsOffStage(Vector2 position, const std::vector<Platform>& platforms) {
    // Find the main platform (usually the largest one at the bottom)
    Rectangle mainPlatform = platforms[0].rect;
    float largestArea = mainPlatform.width * mainPlatform.height;
    
    for (size_t i = 1; i < platforms.size(); i++) {
        float area = platforms[i].rect.width * platforms[i].rect.height;
        if (area > largestArea) {
            mainPlatform = platforms[i].rect;
            largestArea = area;
        }
    }
    
    // Check if position is not above main platform
    bool aboveMainStage = (position.x >= mainPlatform.x - 50 &&
                          position.x <= mainPlatform.x + mainPlatform.width + 50 &&
                          position.y < mainPlatform.y);

    // Check if position is beyond blastzones with a margin
    bool nearBlastzone = (position.x < BLAST_ZONE_LEFT + 100 ||
                          position.x > BLAST_ZONE_RIGHT - 100 ||
                          position.y < BLAST_ZONE_TOP + 100 ||
                          position.y > BLAST_ZONE_BOTTOM - 100);

    return !aboveMainStage || nearBlastzone;
}

// Calculate threat level from player
void EnhancedEnemyAI::UpdateThreatLevel(Character* player, float absDistanceX, float absDistanceY) {
    // Base threat level depends on distance
    float distanceThreat = 1.0f - (std::min(absDistanceX, 500.0f) / 500.0f);
    
    // Attack threat - analyze based on attack type and frame
    float attackThreat = 0.0f;
    if (player->isAttacking) {
        // Different attacks have different threat levels
        switch (player->currentAttack) {
            case Character::FORWARD_SMASH:
            case Character::UP_SMASH:
            case Character::DOWN_SMASH:
                attackThreat = 0.8f; // Smash attacks are very threatening
                break;
                
            case Character::FORWARD_AIR:
            case Character::BACK_AIR:
            case Character::UP_AIR:
            case Character::DOWN_AIR:
                attackThreat = 0.6f; // Aerials are moderately threatening
                break;
                
            case Character::NEUTRAL_SPECIAL:
            case Character::SIDE_SPECIAL:
            case Character::UP_SPECIAL:
            case Character::DOWN_SPECIAL:
                attackThreat = 0.7f; // Specials can be dangerous
                break;
                
            case Character::GRAB:
                attackThreat = 0.75f; // Grabs are threatening, especially at high damage
                break;
                
            default:
                attackThreat = 0.5f; // Standard threat for other attacks
        }
        
        // Adjust based on attack frame - peak threat is during active frames
        int attackDuration = player->attackDuration;
        int activeFramesStart = attackDuration * 0.2f; // Approximate start of active frames
        int activeFramesEnd = attackDuration * 0.6f;   // Approximate end of active frames
        
        if (player->attackFrame < activeFramesStart) {
            // Startup frames - increasing threat
            attackThreat *= (float)player->attackFrame / activeFramesStart;
        } else if (player->attackFrame > activeFramesEnd) {
            // Endlag frames - decreasing threat
            attackThreat *= 1.0f - ((float)(player->attackFrame - activeFramesEnd) / (attackDuration - activeFramesEnd));
        }
    }
    
    // Damage threat - higher at critical percentages
    float damageThreat = 0.0f;
    if (aiState.stockAdvantage < 0) {
        // We're behind on stocks, be more cautious
        damageThreat = std::min(1.0f, player->damagePercent / 80.0f);
    } else {
        // Normal damage threat calculation
        damageThreat = std::min(1.0f, player->damagePercent / 120.0f);
    }
    
    // Position threat - being cornered is dangerous
    float positionThreat = 0.0f;
    if (aiState.nearLeftEdge || aiState.nearRightEdge) {
        positionThreat = 0.3f;
        
        // Even more threatening if player is between AI and center stage
        if ((aiState.nearLeftEdge && player->position.x > player->position.x) ||
            (aiState.nearRightEdge && player->position.x < player->position.x)) {
            positionThreat = 0.6f;
        }
    }
    
    // Combine threat factors with weighted importance
    aiState.threatLevel = (distanceThreat * 0.3f) + 
                         (attackThreat * 0.4f) + 
                         (damageThreat * 0.2f) + 
                         (positionThreat * 0.1f);
                         
    // Add random noise based on difficulty (lower difficulty = more inconsistent threat assessment)
    float randomFactor = (1.0f - difficulty) * 0.2f * ((float)GetRandomValue(-100, 100) / 100.0f);
    aiState.threatLevel = std::min(1.0f, std::max(0.0f, aiState.threatLevel + randomFactor));
}

// Update zone awareness for positional strategy
void EnhancedEnemyAI::UpdateZoneAwareness(Character* enemy, Character* player, const std::vector<Platform>& platforms) {
    // Main stage boundaries (approximate)
    float leftEdge = BLAST_ZONE_LEFT + 150;
    float rightEdge = BLAST_ZONE_RIGHT - 150;
    float stageWidth = rightEdge - leftEdge;
    
    // Update edge proximity
    aiState.nearLeftEdge = (enemy->position.x < leftEdge + stageWidth * 0.2f);
    aiState.nearRightEdge = (enemy->position.x > rightEdge - stageWidth * 0.2f);
    
    // Update vertical positioning
    aiState.abovePlayer = (enemy->position.y < player->position.y - 30);
    aiState.belowPlayer = (enemy->position.y > player->position.y + 30);
    
    // Define strategic zones if not already defined
    if (zoneStrategies.empty()) {
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
            Character::JAB,
            Character::FORWARD_TILT,
            Character::UP_TILT,
            Character::DOWN_TILT
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
            Character::FORWARD_SMASH,
            Character::DOWN_SMASH,
            Character::BACK_AIR
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
            Character::FORWARD_SMASH,
            Character::DOWN_SMASH, 
            Character::BACK_AIR
        };
        rightEdgeZone.priorityMultiplier = 1.0f;
        zoneStrategies.push_back(rightEdgeZone);
        
        // Top platform zone
        // Find highest platform
        Rectangle topPlatform = {0, 0, 0, 0};
        float highestY = SCREEN_HEIGHT;
        
        for (const auto& platform : platforms) {
            if (platform.rect.y < highestY) {
                highestY = platform.rect.y;
                topPlatform = platform.rect;
            }
        }
        
        if (topPlatform.width > 0) {
            ZoneStrategy topZone;
            topZone.zone = {
                topPlatform.x - 50,
                topPlatform.y - 100,
                topPlatform.width + 100,
                150
            };
            topZone.preferredState = EnhancedAIState::PRESSURE;
            topZone.preferredAttacks = {
                Character::UP_AIR,
                Character::NEUTRAL_AIR,
                Character::DOWN_AIR
            };
            topZone.priorityMultiplier = 1.1f;
            zoneStrategies.push_back(topZone);
        }
    }
}

// Determine the best AI state based on current situation
void EnhancedEnemyAI::DetermineAIState(float absDistanceX, float absDistanceY, std::vector<Character*>& players, const std::vector<Platform>& platforms) {
    // Skip state transition if reaction delay hasn't elapsed
    // This simulates human reaction time
    int reactionDelay = static_cast<int>(reactionTimeBase + (GetRandomValue(0, 100) / 100.0f) * reactionTimeVariance * (1.0f - difficulty));
    
    if (aiState.stateTimer < reactionDelay && 
        aiState.currentState != EnhancedAIState::RECOVER &&
        aiState.currentState != EnhancedAIState::COMBO) {
        return;
    }
    
    Character* player = players[0];
    Character* enemy = players[1];

    // Store potential state transitions with their priority scores
    std::vector<std::pair<EnhancedAIState::State, float>> stateOptions;
    
    // Calculate stock and damage advantage
    aiState.stockAdvantage = enemy->stocks - player->stocks;
    aiState.damageAdvantage = player->damagePercent - enemy->damagePercent;
    
    // ==== Calculate priority scores for each potential state ====
    
    // RECOVER - highest priority if off stage
    if (aiState.isOffStage) {
        stateOptions.push_back({EnhancedAIState::RECOVER, 10.0f});
    }
    
    // EDGE_GUARD - high priority if player is off stage and we're not
    if (aiState.playerIsOffStage && !aiState.isOffStage) {
        // Higher priority if player is at high damage
        float edgeGuardPriority = 7.0f + (player->damagePercent / 200.0f) * 2.0f;
        stateOptions.push_back({EnhancedAIState::EDGE_GUARD, edgeGuardPriority});
    }
    
    // LEDGE_TRAP - if player is at ledge but not fully off stage
    bool playerAtLedge = (player->position.x < BLAST_ZONE_LEFT + 200 || player->position.x > BLAST_ZONE_RIGHT - 200) && 
                         !aiState.playerIsOffStage;
    if (playerAtLedge && !aiState.isOffStage) {
        stateOptions.push_back({EnhancedAIState::LEDGE_TRAP, 6.5f});
    }
    
    // COMBO - high priority if we can execute a combo
    if (player->isHitstun && AttemptCombo(enemy, player)) {
        stateOptions.push_back({EnhancedAIState::COMBO, 9.0f});
    }
    
    // DEFEND - priority based on threat level and player's attack state
    if (player->isAttacking && absDistanceX < 120 && absDistanceY < 100) {
        float defendPriority = 5.0f + aiState.threatLevel * 4.0f;
        
        // Adjust based on frame advantage of player's attack
        if (player->attackFrame > player->attackDuration * 0.7f) {
            // Player is in endlag, less need to defend
            defendPriority *= 0.5f;
        }
        
        stateOptions.push_back({EnhancedAIState::DEFEND, defendPriority});
    }
    
    // PUNISH - if player is in endlag or missed an attack
    bool playerInEndlag = player->isAttacking && player->attackFrame > player->attackDuration * 0.6f;
    if (playerInEndlag && absDistanceX < 150 && absDistanceY < 100) {
        float punishPriority = 8.0f;
        stateOptions.push_back({EnhancedAIState::PUNISH, punishPriority});
    }
    
    // ATTACK - priority based on position and damage
    if (absDistanceX < 80 && absDistanceY < 60) {
        float attackPriority = 6.0f;
        
        // Increase priority if player is at high damage (for KO potential)
        if (player->damagePercent > 100) {
            attackPriority += 2.0f;
        }
        
        // Reduce priority if player is shielding
        if (player->isShielding) {
            attackPriority *= 0.5f;
        }
        
        stateOptions.push_back({EnhancedAIState::ATTACK, attackPriority});
    }
    
    // PRESSURE - maintain offensive advantage
    if (aiState.damageAdvantage > 30 && absDistanceX < 150) {
        float pressurePriority = 5.0f + (aiState.damageAdvantage / 200.0f) * 3.0f;
        stateOptions.push_back({EnhancedAIState::PRESSURE, pressurePriority});
    }
    
    // BAIT - if player tends to attack predictably or shield a lot
    if (aiState.playerAttackFrequency[player->currentAttack] > 5 || aiState.playerShieldsOften) {
        float baitPriority = 4.0f;
        
        // Increase if player is aggressive
        if (aiState.playerAggressionLevel > 0.7f) {
            baitPriority += 1.5f;
        }
        
        stateOptions.push_back({EnhancedAIState::BAIT, baitPriority});
    }
    
    // RETREAT - priority based on damage and risk
    if (enemy->damagePercent > 100 || aiState.threatLevel > 0.8f) {
        float retreatPriority = 4.0f + enemy->damagePercent / 50.0f;
        
        // Higher priority if at stock disadvantage
        if (aiState.stockAdvantage < 0) {
            retreatPriority += 2.0f;
        }
        
        stateOptions.push_back({EnhancedAIState::RETREAT, retreatPriority});
    }
    
    // APPROACH - default option, priority based on stage position
    {
        float approachPriority = 3.0f;
        
        // Increase priority if center stage control is important
        bool playerHasCenter = abs(player->position.x - SCREEN_WIDTH/2) < abs(enemy->position.x - SCREEN_WIDTH/2);
        if (playerHasCenter && aiState.centerControlImportance > 0.5f) {
            approachPriority += 2.0f;
        }
        
        stateOptions.push_back({EnhancedAIState::APPROACH, approachPriority});
    }
    
    // NEUTRAL - when assessing the situation
    if (absDistanceX > 200 || (enemy->state == Character::IDLE && GetRandomValue(0, 100) < 10)) {
        float neutralPriority = 2.0f;
        stateOptions.push_back({EnhancedAIState::NEUTRAL, neutralPriority});
    }
    
    // Add risk adjustment to all options
    for (auto& option : stateOptions) {
        float risk = AssessRisk(enemy, player, option.first);
        float reward = PredictReward(enemy, player, option.first);
        
        // Risk-reward calculation
        float riskTolerance = aiState.riskTolerance;
        
        // Adjust risk tolerance based on stock advantage
        if (aiState.stockAdvantage > 0) {
            // If ahead, be more conservative
            riskTolerance *= 0.8f;
        } else if (aiState.stockAdvantage < 0) {
            // If behind, be more aggressive
            riskTolerance *= 1.3f;
        }
        
        // Final adjustment to priority based on risk and reward
        option.second = option.second * (1.0f - (risk * (1.0f - riskTolerance))) * (0.5f + reward * 0.5f);
    }
    
    // Apply random adjustment based on difficulty
    if (difficulty < 1.0f) {
        for (auto& option : stateOptions) {
            float randomAdjust = (1.0f - difficulty) * 3.0f * ((float)GetRandomValue(-100, 100) / 100.0f);
            option.second += randomAdjust;
        }
    }
    
    // Find highest priority state
    EnhancedAIState::State newState = aiState.currentState; // Default to current state
    float highestPriority = 0.0f;
    
    for (const auto& option : stateOptions) {
        if (option.second > highestPriority) {
            highestPriority = option.second;
            newState = option.first;
        }
    }
    
    // Only change state if the priority difference is significant
    // This prevents rapid state switching
    if (newState != aiState.currentState) {
        aiState.currentState = newState;
        aiState.stateTimer = 0;
    }
}

// Risk assessment for a potential state
float EnhancedEnemyAI::AssessRisk(Character* enemy, Character* player, EnhancedAIState::State potentialState) {
    float risk = 0.0f;
    
    // Base risk factors
    switch (potentialState) {
        case EnhancedAIState::EDGE_GUARD:
            // Risky if player has good recovery or we're at high damage
            risk = 0.6f + (enemy->damagePercent / 200.0f) * 0.3f;
            break;
            
        case EnhancedAIState::ATTACK:
            // Risk based on player's shield and reaction patterns
            risk = 0.4f;
            if (player->isShielding) risk += 0.3f;
            if (aiState.playerDefenseLevel > 0.7f) risk += 0.2f;
            break;
            
        case EnhancedAIState::COMBO:
            // Risky if player has good combo-breaking habits
            risk = 0.3f;
            if (aiState.playerJumpsOutOfCombos) risk += 0.3f;
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
    if (player->isAttacking) {
        // Higher risk if player is in active attack frames
        int activeStart = player->attackDuration * 0.2f;
        int activeEnd = player->attackDuration * 0.6f;

        if (player->attackFrame >= activeStart && player->attackFrame <= activeEnd) {
            risk += 0.2f;
        }
    }

    // Adjust for positional disadvantage
    if ((aiState.nearLeftEdge && player->position.x > enemy->position.x) ||
        (aiState.nearRightEdge && player->position.x < enemy->position.x)) {
        risk += 0.15f;
    }

    // Clamp risk value
    return std::min(1.0f, std::max(0.0f, risk));
}

// Predict potential reward for a state
float EnhancedEnemyAI::PredictReward(Character* enemy, Character* player, EnhancedAIState::State potentialState) {
    float reward = 0.5f; // Default moderate reward

    switch (potentialState) {
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
            reward = 0.5f + aiState.playerAggressionLevel * 0.3f;
            break;

        case EnhancedAIState::DEFEND:
            // Defensive reward depends on threat level
            reward = 0.3f + aiState.threatLevel * 0.6f;
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

    // Store expected reward for debugging
    if (potentialState == aiState.currentState) {
        aiState.expectedReward = reward;
    }

    return std::min(1.0f, std::max(0.0f, reward));
}

// Main update function
void EnhancedEnemyAI::Update(std::vector<Character*>& players, std::vector<Platform>& platforms) {
    // Make sure we have at least two players
    if (players.size() < 2) return;

    Character* player = players[0];
    Character* enemy = players[1];

    // Skip AI update if the enemy is dead or dying
    if (enemy->stocks <= 0 || enemy->isDying) return;

    // Increment frame counter
    frameCount++;

    // Apply directional influence if in hitstun
    if (enemy->isHitstun && enemy->hitstunFrames > 5) {
        ApplyDirectionalInfluence(enemy);
        return;
    }

    // Update AI state timer
    aiState.stateTimer++;
    aiState.adaptiveTimer++;

    // Get positions and calculate distances
    Vector2 playerPos = player->position;
    Vector2 enemyPos = enemy->position;
    float distanceX = playerPos.x - enemyPos.x;
    float distanceY = playerPos.y - enemyPos.y;
    float absDistanceX = std::fabs(distanceX);
    float absDistanceY = std::fabs(distanceY);

    // Check if player or AI is off stage
    aiState.isOffStage = IsOffStage(enemyPos, platforms);
    aiState.playerIsOffStage = IsOffStage(playerPos, platforms);

    // Update positional awareness
    UpdateZoneAwareness(enemy, player, platforms);

    // Calculate threat level based on player's state and distance
    UpdateThreatLevel(player, absDistanceX, absDistanceY);

    // Update player history for pattern recognition
    aiState.updateHistory(player, frameCount);

    // Analyze player patterns every 60 frames
    if (frameCount % 60 == 0) {
        aiState.analyzePlayerPatterns();
    }

    // State transitions - determine the best AI state
    DetermineAIState(absDistanceX, absDistanceY, players, platforms);

    // Execute behavior based on current state
    switch (aiState.currentState) {
        case EnhancedAIState::NEUTRAL:
            ExecuteNeutralBehavior(enemy, player);
            break;

        case EnhancedAIState::APPROACH:
            ExecuteApproachBehavior(distanceX, distanceY, absDistanceX, absDistanceY, enemy);
            break;

        case EnhancedAIState::ATTACK:
            ExecuteAttackBehavior(distanceX, distanceY, absDistanceX, absDistanceY, enemy, player);
            break;

        case EnhancedAIState::PRESSURE:
            ExecutePressureBehavior(distanceX, distanceY, enemy, player);
            break;

        case EnhancedAIState::BAIT:
            ExecuteBaitBehavior(distanceX, distanceY, enemy, player);
            break;

        case EnhancedAIState::DEFEND:
            ExecuteDefendBehavior(distanceX, distanceY, enemy, player);
            break;

        case EnhancedAIState::PUNISH:
            ExecutePunishBehavior(distanceX, distanceY, enemy, player);
            break;

        case EnhancedAIState::RECOVER:
            ExecuteRecoverBehavior(distanceX, absDistanceX, enemy, platforms);
            break;

        case EnhancedAIState::RETREAT:
            ExecuteRetreatBehavior(distanceX, enemy, player);
            break;

        case EnhancedAIState::EDGE_GUARD:
            ExecuteEdgeGuardBehavior(playerPos, enemyPos, enemy, player);
            break;

        case EnhancedAIState::LEDGE_TRAP:
            ExecuteLedgeTrapBehavior(playerPos, enemyPos, enemy, player);
            break;

        case EnhancedAIState::COMBO:
            ExecuteComboBehavior(distanceX, distanceY, enemy, player);
            break;
    }
}

// Apply directional influence when in hitstun
void EnhancedEnemyAI::ApplyDirectionalInfluence(Character* enemy) {
    // Apply optimal DI based on knockback direction

    // When getting knocked horizontally, DI upward to survive longer
    if (std::fabs(enemy->velocity.x) > std::fabs(enemy->velocity.y)) {
        if (enemy->velocity.x > 0) {
            // Being knocked right
            enemy->velocity.y -= 0.2f * difficulty;
            enemy->velocity.x -= 0.05f * difficulty;
        } else {
            // Being knocked left
            enemy->velocity.y -= 0.2f * difficulty;
            enemy->velocity.x += 0.05f * difficulty;
        }
    }
    // When getting knocked vertically, DI horizontally to survive longer
    else if (std::fabs(enemy->velocity.y) > std::fabs(enemy->velocity.x)) {
        if (enemy->velocity.y < 0) {
            // Being knocked up
            if (enemy->position.x < SCREEN_WIDTH / 2) {
                enemy->velocity.x += 0.2f * difficulty;
            } else {
                enemy->velocity.x -= 0.2f * difficulty;
            }
        }
    }

    // Wiggle inputs to escape grabs faster (if grabbed)
    if (enemy->isHitstun && GetRandomValue(0, 100) < 80 * difficulty) {
        // Simulate directional inputs to escape grab faster
        int escapeAction = GetRandomValue(0, 3);
        switch (escapeAction) {
            case 0: enemy->velocity.x += 0.1f; break;
            case 1: enemy->velocity.x -= 0.1f; break;
            case 2: enemy->velocity.y -= 0.1f; break;
            case 3: enemy->velocity.y += 0.1f; break;
        }
    }
}

// Choose best attack based on situation
Character::AttackType EnhancedEnemyAI::ChooseBestAttack(Character* enemy, Character* player, float distanceX, float distanceY) {
    // Define utility values for different attacks
    std::vector<std::pair<Character::AttackType, float>> attackUtilities;

    // Check each attack's utility in current situation
    for (const auto& attackOption : attackOptions) {
        if (attackOption->IsViable(distanceX, distanceY, enemy)) {
            float utility = attackOption->GetUtility(distanceX, distanceY, enemy, player);
            attackUtilities.push_back({attackOption->GetAttackType(), utility});
        }
    }

    // Add fallback options if no viable attacks
    if (attackUtilities.empty()) {
        // Default to jab if in range
        if (std::fabs(distanceX) < 80.0f && std::fabs(distanceY) < 40.0f) {
            return Character::JAB;
        }
        // Default to neutral special if at a distance
        else {
            return Character::NEUTRAL_SPECIAL;
        }
    }

    // Sort by utility (descending)
    std::sort(attackUtilities.begin(), attackUtilities.end(),
        [](const std::pair<Character::AttackType, float>& a, const std::pair<Character::AttackType, float>& b) { return a.second > b.second; });

    // At lower difficulties, introduce randomness to attack selection
    if (difficulty < 1.0f && attackUtilities.size() > 1) {
        // Chance to pick suboptimal attack increases as difficulty decreases
        if (GetRandomValue(0, 100) < (1.0f - difficulty) * 40.0f) {
            // Pick randomly from top 3 options or all options if fewer than 3
            int randomIndex = GetRandomValue(0, std::min(2, (int)attackUtilities.size() - 1));
            return attackUtilities[randomIndex].first;
        }
    }

    // Return best attack
    return attackUtilities[0].first;
}

// Execute specific attack based on attack type
void EnhancedEnemyAI::ExecuteNeutralBehavior(Character* enemy, Character* player) {
    // Neutral behavior: observe and position strategically

    // Move toward center stage if far from it
    float centerX = SCREEN_WIDTH / 2;
    if (enemy->position.x < centerX - 50) {
        enemy->moveRight();
        enemy->isFacingRight = true;
    } else if (enemy->position.x > centerX + 50) {
        enemy->moveLeft();
        enemy->isFacingRight = false;
    }

    // Face the player
    enemy->isFacingRight = (player->position.x > enemy->position.x);

    // Occasionally shield preemptively
    if (GetRandomValue(0, 100) < 5 && aiState.playerAggressionLevel > 0.6f) {
        enemy->shield();
    }

    // Occasionally jump to platforms
    if (GetRandomValue(0, 100) < 3 && enemy->state != Character::JUMPING) {
        enemy->jump();
    }

    // Transition out of neutral state after a while
    if (aiState.stateTimer > 60) {
        aiState.currentState = EnhancedAIState::APPROACH;
        aiState.stateTimer = 0;
    }
}

// Execute approach behavior
void EnhancedEnemyAI::ExecuteApproachBehavior(float distanceX, float distanceY, float absDistanceX, float absDistanceY, Character* enemy) {
    // Smart approach with spacing awareness

    // Optimal spacing distance for approaching
    float optimalSpace = 70.0f;

    // If we're at optimal spacing, we might want to attack or shield
    if (absDistanceX < optimalSpace + 10 && absDistanceX > optimalSpace - 10) {
        // At optimal spacing, consider attacking or defending
        if (GetRandomValue(0, 100) < 30 * difficulty) {
            aiState.currentState = EnhancedAIState::ATTACK;
            return;
        }
    }

    // Otherwise approach intelligently
    if (distanceX > optimalSpace) {
        enemy->moveRight();
        enemy->isFacingRight = true;
    } else if (distanceX < -optimalSpace) {
        enemy->moveLeft();
        enemy->isFacingRight = false;
    }

    // Jump to approach when needed
    if (distanceY < -80 && absDistanceX < 150 &&
        GetRandomValue(0, 100) > 70 && enemy->state != Character::JUMPING) {
        enemy->jump();
    }

    // Dash dance near optimal spacing (advanced technique)
    if (absDistanceX < optimalSpace + 30 && absDistanceX > optimalSpace - 30) {
        if (GetRandomValue(0, 100) < 15 * difficulty) {
            // Briefly dash in opposite direction
            if (distanceX > 0) {
                enemy->moveLeft();
                // Quick reversal to maintain facing
                if (GetRandomValue(0, 100) < 80) {
                    enemy->isFacingRight = true;
                }
            } else {
                enemy->moveRight();
                if (GetRandomValue(0, 100) < 80) {
                    enemy->isFacingRight = false;
                }
            }
        }
    }

    // Short hop aerials approach (advanced technique)
    if (absDistanceX < 130 && absDistanceY < 50 &&
        GetRandomValue(0, 100) > 80 && enemy->state != Character::JUMPING) {
        enemy->jump();
        // Will execute an aerial in the next frame
        if (GetRandomValue(0, 100) < 50 * difficulty) {
            aiState.currentState = EnhancedAIState::ATTACK;
        }
    }

    // Fast fall when above target position to close vertical distance
    if (distanceY > 30 && enemy->velocity.y > 0) {
        enemy->fastFall();
    }
}

// Combo detection
bool EnhancedEnemyAI::AttemptCombo(Character* enemy, Character* player) {
    // Check if we can start or continue a combo
    if (!player->isHitstun) return false;

    // Very basic combo system - will be expanded
    float playerDamage = player->damagePercent;

    // Simple combo flowchart based on damage percentage
    if (playerDamage < 40) {
        // Low damage combos (e.g., uptilt chains, fair strings)
        aiState.currentCombo.sequence = {Character::UP_TILT, Character::UP_TILT, Character::UP_AIR};
        aiState.currentCombo.startingDamage = playerDamage;
        aiState.currentCombo.isFinisher = false;
        aiState.currentCombo.hitstunRemaining = player->hitstunFrames;
        return true;
    }
    else if (playerDamage < 80) {
        // Mid damage combos
        aiState.currentCombo.sequence = {Character::NEUTRAL_AIR, Character::FORWARD_AIR, Character::UP_SPECIAL};
        aiState.currentCombo.startingDamage = playerDamage;
        aiState.currentCombo.isFinisher = false;
        aiState.currentCombo.hitstunRemaining = player->hitstunFrames;
        return true;
    }
    else {
        // High damage finishers
        aiState.currentCombo.sequence = {Character::UP_SMASH};
        aiState.currentCombo.startingDamage = playerDamage;
        aiState.currentCombo.isFinisher = true;
        aiState.currentCombo.hitstunRemaining = player->hitstunFrames;
        return true;
    }

    return false;
}

// Execute attack behavior
void EnhancedEnemyAI::ExecuteAttackBehavior(float distanceX, float distanceY, float absDistanceX, float absDistanceY, Character* enemy, Character* player) {
    // Face the player
    enemy->isFacingRight = (distanceX > 0);

    // Track time since last attack to prevent button mashing
    bool canAttackNow = (aiState.stateTimer - aiState.lastAttackFrame > 15);

    if (!canAttackNow) return;

    // Choose optimal attack based on position and enemy state
    Character::AttackType attackChoice = ChooseBestAttack(enemy, player, distanceX, distanceY);

    // Execute the chosen attack
    switch (attackChoice) {
        case Character::JAB:
            enemy->jab();
            break;
        case Character::FORWARD_TILT:
            enemy->forwardTilt();
            break;
        case Character::UP_TILT:
            enemy->upTilt();
            break;
        case Character::DOWN_TILT:
            enemy->downTilt();
            break;
        case Character::DASH_ATTACK:
            enemy->dashAttack();
            break;
        case Character::FORWARD_SMASH:
            enemy->forwardSmash(GetRandomValue(10, 25) * difficulty);
            break;
        case Character::UP_SMASH:
            enemy->upSmash(GetRandomValue(10, 25) * difficulty);
            break;
        case Character::DOWN_SMASH:
            enemy->downSmash(GetRandomValue(10, 25) * difficulty);
            break;
        case Character::NEUTRAL_AIR:
            enemy->neutralAir();
            break;
        case Character::FORWARD_AIR:
            enemy->forwardAir();
            break;
        case Character::BACK_AIR:
            enemy->backAir();
            break;
        case Character::UP_AIR:
            enemy->upAir();
            break;
        case Character::DOWN_AIR:
            enemy->downAir();
            break;
        case Character::NEUTRAL_SPECIAL:
            enemy->neutralSpecial();
            break;
        case Character::SIDE_SPECIAL:
            enemy->sideSpecial();
            break;
        case Character::UP_SPECIAL:
            enemy->upSpecial();
            break;
        case Character::DOWN_SPECIAL:
            enemy->downSpecial();
            break;
        case Character::GRAB:
            enemy->grab();
            break;
        default:
            // If no specific attack was chosen, do a jab as fallback
            enemy->jab();
            break;
    }

    aiState.lastAttackFrame = aiState.stateTimer;

    // Handle throws if grabbing
    if (enemy->isGrabbing) {
        int throwChoice = GetRandomValue(0, 100);

        // Choose optimal throw based on position and damage
        if (player->damagePercent > 100 && enemy->position.x < 200) {
            // Back throw for KO when near edge
            enemy->backThrow();
        } else if (player->damagePercent > 100 && enemy->position.x > SCREEN_WIDTH - 200) {
            // Forward throw for KO when near edge
            enemy->forwardThrow();
        } else if (player->damagePercent < 50) {
            // Down throw for combos at low percent
            enemy->downThrow();
        } else {
            // Up throw at mid percent
            enemy->upThrow();
        }
    }
}

// Execute pressure behavior - maintaining offensive advantage
void EnhancedEnemyAI::ExecutePressureBehavior(float distanceX, float distanceY, Character* enemy, Character* player) {
    // Face the player
    enemy->isFacingRight = (distanceX > 0);

    // Pressure involves staying close and using safe moves to maintain advantage

    // Keep optimal pressure distance
    float pressureDistance = 50.0f;
    if (std::fabs(distanceX) > pressureDistance + 20) {
        // Move toward player if too far
        if (distanceX > 0) {
            enemy->moveRight();
        } else {
            enemy->moveLeft();
        }
    } else if (std::fabs(distanceX) < pressureDistance - 20) {
        // Back up slightly if too close
        if (distanceX > 0) {
            enemy->moveLeft();
            enemy->isFacingRight = true; // Still face player
        } else {
            enemy->moveRight();
            enemy->isFacingRight = false; // Still face player
        }
    }

    // Use safe, quick attacks to maintain pressure
    if (aiState.stateTimer % 20 == 0) {
        int attackChoice = GetRandomValue(0, 100);

        if (attackChoice < 30) {
            enemy->jab();
        } else if (attackChoice < 50) {
            enemy->forwardTilt();
        } else if (attackChoice < 70) {
            enemy->downTilt();
        } else if (attackChoice < 85 && enemy->state == Character::JUMPING) {
            enemy->neutralAir();
        } else if (attackChoice < 95) {
            // Occasionally grab to mix up pressure
            enemy->grab();
        }
    }

    // Shield if player attacks
    if (player->isAttacking && aiState.stateTimer % 15 == 0) {
        enemy->shield();
    } else if (enemy->isShielding && !player->isAttacking) {
        enemy->releaseShield();
    }

    // Jump to follow player on platforms
    if (distanceY < -50 && aiState.stateTimer % 30 == 0) {
        enemy->jump();
    }

    // Change state if pressure has lasted too long
    if (aiState.stateTimer > 120) {
        // Either reset to neutral or go for a stronger attack
        if (GetRandomValue(0, 100) < 50) {
            aiState.currentState = EnhancedAIState::NEUTRAL;
        } else {
            aiState.currentState = EnhancedAIState::ATTACK;
        }
        aiState.stateTimer = 0;
    }
}

// Execute baiting behavior to provoke and punish player attacks
void EnhancedEnemyAI::ExecuteBaitBehavior(float distanceX, float distanceY, Character* enemy, Character* player) {
    // Face the player
    enemy->isFacingRight = (distanceX > 0);

    // Dash dance (move back and forth) to bait attacks
    int dashFrame = aiState.stateTimer % 20;

    if (dashFrame < 10) {
        // Dash in one direction
        if (distanceX > 0) {
            enemy->moveRight();
        } else {
            enemy->moveLeft();
        }
    } else {
        // Dash in opposite direction
        if (distanceX > 0) {
            enemy->moveLeft();
            enemy->isFacingRight = true; // Maintain facing toward player
        } else {
            enemy->moveRight();
            enemy->isFacingRight = false; // Maintain facing toward player
        }
    }

    // Empty short hops to bait anti-air responses
    if (aiState.stateTimer % 45 == 0 && enemy->state != Character::JUMPING) {
        enemy->jump();
        // Don't attack, just empty hop
    }

    // Shield briefly and then drop shield to bait grabs
    if (aiState.stateTimer % 60 == 0) {
        enemy->shield();
    } else if (enemy->isShielding && aiState.stateTimer % 60 == 10) {
        enemy->releaseShield();
    }

    // Feint attacks (start dash but don't attack) to bait defensive options
    if (aiState.stateTimer % 40 == 0) {
        // Dash toward player but don't commit to attack
        if (distanceX > 0) {
            enemy->moveRight();
        } else {
            enemy->moveLeft();
        }
    }

    // If player attacks while we're baiting, switch to defensive
    if (player->isAttacking && player->attackFrame < 5) {
        aiState.currentState = EnhancedAIState::DEFEND;
        aiState.stateTimer = 0;
    }

    // If baiting for too long with no response, switch to approaching
    if (aiState.stateTimer > 180) {
        aiState.currentState = EnhancedAIState::APPROACH;
        aiState.stateTimer = 0;
    }
}

// Execute defensive options with optimal timing
void EnhancedEnemyAI::ExecuteDefendBehavior(float distanceX, float distanceY, Character* enemy, Character* player) {
    // Face the player
    enemy->isFacingRight = (distanceX > 0);

    // If player is attacking, choose best defensive option
    if (player->isAttacking) {
        int defenseChoice = GetRandomValue(0, 100);

        // Perfect shielding (precise timing)
        if (defenseChoice < 40 * difficulty && std::fabs(distanceX) < 80 && std::fabs(distanceY) < 60) {
            enemy->shield();
        }
        // Spotdodge (for close attacks, especially grabs)
        else if (defenseChoice < (40 + 20 * difficulty) && std::fabs(distanceX) < 50 && std::fabs(distanceY) < 40) {
            enemy->spotDodge();
        }
        // Roll away from danger
        else if (defenseChoice < (60 + 25 * difficulty)) {
            if (distanceX > 0) {
                enemy->backDodge(); // Roll away if player is to the right
            } else {
                enemy->forwardDodge(); // Roll away if player is to the left
            }
        }
        // Jump away (especially good for avoiding ground attacks)
        else if (enemy->state != Character::JUMPING && enemy->state != Character::FALLING) {
            enemy->jump();

            // Air dodge if needed after jump
            if (GetRandomValue(0, 100) > 70) {
                float dodgeX = (distanceX > 0) ? -1.0f : 1.0f;
                float dodgeY = -0.5f;
                enemy->airDodge(dodgeX, dodgeY);
            }
        }
    }
    // If player is grabbing or has grabbed, escape
    else if (player->isGrabbing) {
        // Button mashing to escape grabs faster
        int escapeAction = GetRandomValue(0, 3);
        switch (escapeAction) {
            case 0: enemy->moveLeft(); break;
            case 1: enemy->moveRight(); break;
            case 2: enemy->jump(); break;
            case 3: enemy->shield(); enemy->releaseShield(); break;
        }
    }
    // If shielding and player isn't attacking anymore, release shield
    else if (enemy->isShielding && !player->isAttacking) {
        enemy->releaseShield();

        // Shield grab if player is close and in endlag
        if (std::fabs(distanceX) < 60 && std::fabs(distanceY) < 40 &&
            player->isAttacking && player->attackFrame > player->attackDuration * 0.6f) {
            aiState.currentState = EnhancedAIState::PUNISH;
            aiState.stateTimer = 0;
        }
    }
    // If state has lasted too long, return to neutral
    else if (aiState.stateTimer > 60) {
        aiState.currentState = EnhancedAIState::NEUTRAL;
        aiState.stateTimer = 0;
    }
}

// Execute punish behavior after successful defense or read
void EnhancedEnemyAI::ExecutePunishBehavior(float distanceX, float distanceY, Character* enemy, Character* player) {
    // Face the player
    enemy->isFacingRight = (distanceX > 0);

    // Punish options vary by player state and distance

    // For successful shield punish when player is in endlag
    if (player->isAttacking && player->attackFrame > player->attackDuration * 0.6f) {
        // Close range punishes
        if (std::fabs(distanceX) < 50 && std::fabs(distanceY) < 40) {
            // Grab punish
            if (GetRandomValue(0, 100) < 70 * difficulty) {
                enemy->grab();
            } else {
                // Up smash out of shield
                enemy->upSmash(10 * difficulty);
            }
        }
        // Mid range punishes
        else if (std::fabs(distanceX) < 120 && std::fabs(distanceY) < 60) {
            int punishOption = GetRandomValue(0, 100);
            if (punishOption < 40) {
                enemy->dashAttack();
            } else if (punishOption < 70) {
                enemy->forwardSmash(15 * difficulty);
            } else {
                enemy->sideSpecial();
            }
        }
    }
    // If player is in hitstun, follow up with a combo starter
    else if (player->isHitstun) {
        if (AttemptCombo(enemy, player)) {
            aiState.currentState = EnhancedAIState::COMBO;
            aiState.stateTimer = 0;
            return;
        }

        // Otherwise use a strong attack based on percent
        if (player->damagePercent < 50) {
            // Low percent - use combo starters
            if (GetRandomValue(0, 100) < 70) {
                enemy->upTilt();
            } else {
                enemy->grab();
            }
        } else if (player->damagePercent < 100) {
            // Mid percent - use launchers
            if (GetRandomValue(0, 100) < 60) {
                enemy->upAir();
            } else {
                enemy->forwardAir();
            }
        } else {
            // High percent - use KO moves
            if (GetRandomValue(0, 100) < 50) {
                enemy->forwardSmash(20 * difficulty);
            } else {
                enemy->upSmash(20 * difficulty);
            }
        }
    }
    // If punish whiffed or is finished, return to neutral
    else if (aiState.stateTimer > 30) {
        aiState.currentState = EnhancedAIState::NEUTRAL;
        aiState.stateTimer = 0;
    }
}

// Execute combo behavior to string attacks together
void EnhancedEnemyAI::ExecuteComboBehavior(float distanceX, float distanceY, Character* enemy, Character* player) {
    // If player is no longer in hitstun, combo is dropped
    if (!player->isHitstun && aiState.stateTimer > 5) {
        aiState.currentState = EnhancedAIState::NEUTRAL;
        aiState.stateTimer = 0;
        aiState.comboCounter = 0;
        return;
    }

    // Execute current combo if we have one
    if (!aiState.currentCombo.sequence.empty()) {
        // Get next move in sequence
        int comboStep = aiState.comboCounter % aiState.currentCombo.sequence.size();
        Character::AttackType nextMove = aiState.currentCombo.sequence[comboStep];

        // Position for the next combo move
        float optimalDistX = 0.0f;
        float optimalDistY = 0.0f;

        // Set optimal positioning based on next move
        switch (nextMove) {
            case Character::UP_TILT:
            case Character::UP_SMASH:
                optimalDistX = 0.0f;
                optimalDistY = 10.0f;
                break;

            case Character::FORWARD_AIR:
                optimalDistX = enemy->isFacingRight ? 40.0f : -40.0f;
                optimalDistY = -20.0f;
                break;

            case Character::UP_AIR:
                optimalDistX = 0.0f;
                optimalDistY = -40.0f;
                break;

            case Character::BACK_AIR:
                optimalDistX = enemy->isFacingRight ? -40.0f : 40.0f;
                optimalDistY = -10.0f;
                break;

            default:
                optimalDistX = 30.0f * (enemy->isFacingRight ? 1.0f : -1.0f);
                optimalDistY = 0.0f;
                break;
        }

        // Move to optimal position
        if (distanceX < optimalDistX - 10) {
            enemy->moveRight();
            enemy->isFacingRight = true;
        } else if (distanceX > optimalDistX + 10) {
            enemy->moveLeft();
            enemy->isFacingRight = false;
        }

        // Jump or fast-fall to get vertical positioning
        if (distanceY < optimalDistY - 10 && enemy->state != Character::JUMPING) {
            enemy->jump();
        } else if (distanceY > optimalDistY + 10 && enemy->velocity.y > 0) {
            enemy->fastFall();
        }

        // Execute the combo move when in position and timing is right
        // Timing depends on hitstun and positioning
        bool inPosition = std::fabs(distanceX - optimalDistX) < 20 && std::fabs(distanceY - optimalDistY) < 20;
        bool correctState = true;

        // Check if we're in the right state for the attack
        if ((nextMove >= Character::NEUTRAL_AIR && nextMove <= Character::DOWN_AIR) &&
            enemy->state != Character::JUMPING && enemy->state != Character::FALLING) {
            correctState = false;
        }

        // Execute attack if conditions are met
        if (inPosition && correctState && aiState.stateTimer % 10 == 0) {
            switch (nextMove) {
                case Character::JAB:
                    enemy->jab();
                    break;
                case Character::FORWARD_TILT:
                    enemy->forwardTilt();
                    break;
                case Character::UP_TILT:
                    enemy->upTilt();
                    break;
                case Character::DOWN_TILT:
                    enemy->downTilt();
                    break;
                case Character::DASH_ATTACK:
                    enemy->dashAttack();
                    break;
                case Character::FORWARD_SMASH:
                    enemy->forwardSmash(10 * difficulty);
                    break;
                case Character::UP_SMASH:
                    enemy->upSmash(10 * difficulty);
                    break;
                case Character::DOWN_SMASH:
                    enemy->downSmash(10 * difficulty);
                    break;
                case Character::NEUTRAL_AIR:
                    enemy->neutralAir();
                    break;
                case Character::FORWARD_AIR:
                    enemy->forwardAir();
                    break;
                case Character::BACK_AIR:
                    enemy->backAir();
                    break;
                case Character::UP_AIR:
                    enemy->upAir();
                    break;
                case Character::DOWN_AIR:
                    enemy->downAir();
                    break;
                case Character::NEUTRAL_SPECIAL:
                    enemy->neutralSpecial();
                    break;
                case Character::SIDE_SPECIAL:
                    enemy->sideSpecial();
                    break;
                case Character::UP_SPECIAL:
                    enemy->upSpecial();
                    break;
                case Character::DOWN_SPECIAL:
                    enemy->downSpecial();
                    break;
                default:
                    break;
            }

            // Increment combo counter
            aiState.comboCounter++;

            // If we've completed the combo, reset state
            if (aiState.comboCounter >= aiState.currentCombo.sequence.size()) {
                if (aiState.currentCombo.isFinisher) {
                    // After finisher, go to neutral
                    aiState.currentState = EnhancedAIState::NEUTRAL;
                } else {
                    // After non-finisher, continue pressure
                    aiState.currentState = EnhancedAIState::PRESSURE;
                }
                aiState.stateTimer = 0;
                aiState.comboCounter = 0;
            }
        }
    }

    // If combo state lasts too long, reset
    if (aiState.stateTimer > 120) {
        aiState.currentState = EnhancedAIState::NEUTRAL;
        aiState.stateTimer = 0;
        aiState.comboCounter = 0;
    }
}

// Execute recovery behavior to get back to stage
void EnhancedEnemyAI::ExecuteRecoverBehavior(float distanceX, float absDistanceX, Character* enemy, const std::vector<Platform>& platforms) {
    // Find the main platform
    Rectangle mainPlatform = platforms[0].rect; // Default to first platform
    float largestArea = mainPlatform.width * mainPlatform.height;

    for (size_t i = 1; i < platforms.size(); i++) {
        float area = platforms[i].rect.width * platforms[i].rect.height;
        if (area > largestArea) {
            mainPlatform = platforms[i].rect;
            largestArea = area;
        }
    }

    // Calculate optimal recovery target point
    float targetX = mainPlatform.x + mainPlatform.width / 2;
    float targetY = mainPlatform.y - 20; // Above platform

    // Calculate angle for recovery
    float recoveryAngle = CalculateRecoveryAngle(enemy, platforms);

    // Extreme danger - low recovery has higher priority
    bool dangerouslyLow = enemy->position.y > BLAST_ZONE_BOTTOM - 200;

    // First priority: get horizontal alignment with stage
    if (enemy->position.x < targetX - 100) {
        enemy->moveRight();
    } else if (enemy->position.x > targetX + 100) {
        enemy->moveLeft();
    }

    // Use resources intelligently for recovery

    // If below stage and has double jump, use it when close enough
    if (enemy->position.y > targetY + 100 && absDistanceX < 350 &&
        enemy->hasDoubleJump && !enemy->isJumping) {
        // For optimal recovery, sometimes delay the double jump
        if (dangerouslyLow || GetRandomValue(0, 100) < 70 * difficulty) {
            enemy->jump(); // This will use double jump if needed
        }
    }

    // Use up special for recovery, but save it for the right moment
    if (enemy->position.y > targetY + 50 &&
        absDistanceX < 300 && !enemy->isJumping && !enemy->hasDoubleJump &&
        enemy->specialUpCD.current <= 0) {

        // Optimal timing based on distance and angle
        if (dangerouslyLow ||
            (std::fabs(enemy->position.x - targetX) < 150 && std::fabs(recoveryAngle) < 0.5f)) {
            enemy->upSpecial();
        }
    }

    // Air dodge as a recovery mixup or extension
    if (enemy->position.y > targetY + 50 &&
        absDistanceX < 250 && !enemy->isJumping && !enemy->hasDoubleJump &&
        enemy->specialUpCD.current > 0 && !enemy->isDodging &&
        GetRandomValue(0, 100) > 30) {

        // Calculate best air dodge angle for recovery
        float dodgeX = (enemy->position.x < targetX) ? 0.8f : -0.8f;
        float dodgeY = -0.6f;
        enemy->airDodge(dodgeX, dodgeY);
    }

    // If close to danger zone, prioritize getting back
    if (enemy->position.y > BLAST_ZONE_BOTTOM - 150) {
        // Maximum effort to recover - mash jump and up special
        if (!enemy->isJumping && enemy->hasDoubleJump) {
            enemy->jump();
        } else if (enemy->specialUpCD.current <= 0) {
            enemy->upSpecial();
        }
    }

    // If recovery was successful, return to neutral state
    bool safelyOnStage = false;
    for (const auto& platform : platforms) {
        if (CheckCollisionRecs(enemy->getRect(), platform.rect)) {
            safelyOnStage = true;
            break;
        }
    }

    if (safelyOnStage && enemy->velocity.y == 0) {
        aiState.currentState = EnhancedAIState::NEUTRAL;
        aiState.stateTimer = 0;
    }
}

// Calculate optimal recovery angle
float EnhancedEnemyAI::CalculateRecoveryAngle(Character* enemy, const std::vector<Platform>& platforms) {
    // Find the closest edge of a platform
    float closestEdgeX = SCREEN_WIDTH / 2;
    float closestEdgeY = SCREEN_HEIGHT;
    float minDist = 999999.0f;

    for (const auto& platform : platforms) {
        // Check left edge
        float leftDist = sqrtf(powf(platform.rect.x - enemy->position.x, 2) +
                              powf(platform.rect.y - enemy->position.y, 2));
        if (leftDist < minDist) {
            minDist = leftDist;
            closestEdgeX = platform.rect.x;
            closestEdgeY = platform.rect.y;
        }

        // Check right edge
        float rightDist = sqrtf(powf(platform.rect.x + platform.rect.width - enemy->position.x, 2) +
                               powf(platform.rect.y - enemy->position.y, 2));
        if (rightDist < minDist) {
            minDist = rightDist;
            closestEdgeX = platform.rect.x + platform.rect.width;
            closestEdgeY = platform.rect.y;
        }
    }

    // Calculate angle to closest edge
    float dx = closestEdgeX - enemy->position.x;
    float dy = closestEdgeY - enemy->position.y;

    return atan2f(dy, dx);
}

// Execute retreat behavior to reset neutral and avoid damage
void EnhancedEnemyAI::ExecuteRetreatBehavior(float distanceX, Character* enemy, Character* player) {
    // Move away from player but keep facing them for defense
    if (distanceX > 0) {
        enemy->moveLeft();
        enemy->isFacingRight = true; // Still face player while retreating
    } else {
        enemy->moveRight();
        enemy->isFacingRight = false; // Still face player while retreating
    }

    // Shield if player approaches too quickly
    if (std::fabs(distanceX) < 100 && player->velocity.x != 0 &&
        ((distanceX > 0 && player->velocity.x > 3) ||
         (distanceX < 0 && player->velocity.x < -3))) {
        enemy->shield();
    }

    // Jump to platform to reset position
    if (aiState.stateTimer % 30 == 0 && GetRandomValue(0, 100) > 50) {
        enemy->jump();
    }

    // Use projectiles to keep player away
    if (aiState.stateTimer % 45 == 0 && GetRandomValue(0, 100) > 40) {
        enemy->neutralSpecial();
    }

    // Fast fall when above a platform
    if (enemy->velocity.y > 0 && enemy->state == Character::FALLING) {
        enemy->fastFall();
    }

    // Check if we've retreated enough
    if (std::fabs(distanceX) > 300 || aiState.stateTimer > 90) {
        aiState.currentState = EnhancedAIState::NEUTRAL;
        aiState.stateTimer = 0;
    }
}

// Execute edge guarding behavior to prevent player recovery
void EnhancedEnemyAI::ExecuteEdgeGuardBehavior(Vector2 playerPos, Vector2 enemyPos, Character* enemy, Character* player) {
    float distanceX = playerPos.x - enemyPos.x;
    float distanceY = playerPos.y - enemyPos.y;
    float absDistanceX = std::fabs(distanceX);
    float absDistanceY = std::fabs(distanceY);

    // Determine which edge the player is trying to recover to
    float edgeX = (playerPos.x < SCREEN_WIDTH/2) ?
                  SCREEN_WIDTH/2 - 300 :
                  SCREEN_WIDTH/2 + 300;

    // Move toward the edge
    if (enemyPos.x < edgeX - 50) {
        enemy->moveRight();
    } else if (enemyPos.x > edgeX + 50) {
        enemy->moveLeft();
    }

    // Choose edge guarding strategy based on player's position and damage

    // Player is trying to recover from below
    if (playerPos.y > SCREEN_HEIGHT - 150) {
        // If player is far below, prepare for their recovery
        if (playerPos.y > SCREEN_HEIGHT) {
            // Wait at edge
            if (std::fabs(enemyPos.x - edgeX) < 50) {
                // Occasionally charge a smash attack at edge
                if (aiState.stateTimer % 60 == 0 && GetRandomValue(0, 100) > 50) {
                    enemy->downSmash(GetRandomValue(10, 30) * difficulty);
                }

                // Or prepare to intercept with an aerial
                if (aiState.stateTimer % 45 == 0 && GetRandomValue(0, 100) > 60) {
                    // Jump off stage to intercept
                    enemy->jump();
                }
            }
        }
        // Player is close enough to intercept
        else if (absDistanceX < 150 && absDistanceY < 150) {
            // Jump off stage for aggressive edge guard
            if (enemy->state != Character::JUMPING &&
                enemy->state != Character::FALLING &&
                GetRandomValue(0, 100) > 40) {
                enemy->jump();
            }

            // Use appropriate aerial based on position
            if (enemy->state == Character::JUMPING ||
                enemy->state == Character::FALLING) {

                if (distanceY > 0 && absDistanceX < 100) {
                    // Player is below, use down air (spike)
                    enemy->downAir();
                } else if (absDistanceY < 50) {
                    // Player is beside, use back/forward air
                    if ((distanceX > 0 && enemy->isFacingRight) ||
                        (distanceX < 0 && !enemy->isFacingRight)) {
                        enemy->forwardAir();
                    } else {
                        enemy->backAir();
                    }
                }
            }
        }
    }
    // Player is trying to recover from the side
    else if ((playerPos.x < SCREEN_WIDTH/2 - 300 ||
              playerPos.x > SCREEN_WIDTH/2 + 300) &&
              playerPos.y < SCREEN_HEIGHT - 100) {

        // If player is attempting side recovery
        if (std::fabs(playerPos.y - enemyPos.y) < 100) {
            // Use projectiles or side special to intercept
            if (aiState.stateTimer % 30 == 0 && GetRandomValue(0, 100) > 60) {
                enemy->neutralSpecial();
            } else if (aiState.stateTimer % 45 == 0 && GetRandomValue(0, 100) > 70) {
                enemy->sideSpecial();
            }
        }
        // Jump and intercept with aerial
        else if (playerPos.y < enemyPos.y &&
                aiState.stateTimer % 40 == 0 &&
                GetRandomValue(0, 100) > 50) {
            enemy->jump();

            // Wait to use aerial at right time
            if (enemy->state == Character::JUMPING &&
                std::fabs(playerPos.y - enemyPos.y) < 50) {
                if ((distanceX > 0 && enemy->isFacingRight) ||
                    (distanceX < 0 && !enemy->isFacingRight)) {
                    enemy->forwardAir();
                } else {
                    enemy->backAir();
                }
            }
        }
    }

    // 2-frame punish attempt (targeting the ledge grab vulnerability window)
    if (std::fabs(enemyPos.x - edgeX) < 40 &&
        playerPos.y > SCREEN_HEIGHT - 150 &&
        playerPos.y < SCREEN_HEIGHT - 50) {
        if (aiState.stateTimer % 10 <= 2 && GetRandomValue(0, 100) < 70 * difficulty) {
            enemy->downTilt(); // Fast move to hit ledge grab
        }
    }

    // Return to stage if AI is getting too risky with edge guard
    if (aiState.isOffStage &&
        (enemyPos.y > SCREEN_HEIGHT - 100 ||
         enemyPos.x < BLAST_ZONE_LEFT + 150 ||
         enemyPos.x > BLAST_ZONE_RIGHT - 150)) {
        aiState.currentState = EnhancedAIState::RECOVER;
    }

    // If player made it back to stage, return to neutral
    if (!aiState.playerIsOffStage) {
        aiState.currentState = EnhancedAIState::NEUTRAL;
    }
}

// Execute ledge trapping behavior to catch player's ledge options
void EnhancedEnemyAI::ExecuteLedgeTrapBehavior(Vector2 playerPos, Vector2 enemyPos, Character* enemy, Character* player) {
    // Determine which ledge the player is at
    float ledgeX = (playerPos.x < SCREEN_WIDTH/2) ?
                  SCREEN_WIDTH/2 - 300 :
                  SCREEN_WIDTH/2 + 300;

    // Optimal position for ledge trapping (slightly away from ledge)
    float optimalX = ledgeX + (ledgeX < SCREEN_WIDTH/2 ? 80 : -80);

    // Move to optimal position
    if (enemyPos.x < optimalX - 10) {
        enemy->moveRight();
    } else if (enemyPos.x > optimalX + 10) {
        enemy->moveLeft();
    }

    // Always face toward the ledge
    enemy->isFacingRight = (ledgeX > enemyPos.x);

    // Track player's likely ledge option based on past behavior
    bool expectNeutralGetup = false;
    bool expectRollGetup = false;
    bool expectJumpGetup = false;
    bool expectAttackGetup = false;

    // Use simple heuristics for ledge option prediction
    if (aiState.playerFavorsGround) {
        expectRollGetup = true;
    } else if (aiState.playerFavorsAerial) {
        expectJumpGetup = true;
    } else if (aiState.playerAggressionLevel > 0.7f) {
        expectAttackGetup = true;
    } else {
        expectNeutralGetup = true;
    }

    // Apply different ledge trapping strategies based on prediction

    // Dash dance near ledge to bait and react
    if (aiState.stateTimer % 20 < 10) {
        if (enemyPos.x < optimalX) {
            enemy->moveRight();
        } else {
            enemy->moveLeft();
        }
    }

    // React to predicted ledge option
    if (std::fabs(enemyPos.x - optimalX) < 30) {
        if (expectRollGetup && aiState.stateTimer % 45 == 0) {
            // For roll getup, use down smash or move further in
            if (GetRandomValue(0, 100) < 50) {
                enemy->downSmash(20 * difficulty);
            } else {
                // Position for roll punish
                float rollTargetX = ledgeX + (ledgeX < SCREEN_WIDTH/2 ? 150 : -150);
                if (enemyPos.x < rollTargetX) {
                    enemy->moveRight();
                } else {
                    enemy->moveLeft();
                }
            }
        } else if (expectJumpGetup && aiState.stateTimer % 40 == 0) {
            // For jump getup, use up air or up smash
            if (enemy->state != Character::JUMPING && enemy->state != Character::FALLING) {
                enemy->jump();
            } else {
                enemy->upAir();
            }
        } else if (expectAttackGetup && aiState.stateTimer % 35 == 0) {
            // For attack getup, shield then grab
            enemy->shield();
            if (aiState.stateTimer % 35 == 5) {
                enemy->releaseShield();
                enemy->grab();
            }
        } else if (expectNeutralGetup && aiState.stateTimer % 50 == 0) {
            // For neutral getup, time a jab or grab
            if (GetRandomValue(0, 100) < 60) {
                enemy->jab();
            } else {
                enemy->grab();
            }
        }
    }

    // Occasionally throw out a safe move to cover multiple options
    if (aiState.stateTimer % 60 == 0 && GetRandomValue(0, 100) < 40) {
        if (std::fabs(enemyPos.x - ledgeX) < 100) {
            enemy->forwardTilt();
        }
    }

    // If player gets away from ledge, return to neutral game
    if (std::fabs(playerPos.x - ledgeX) > 150 && playerPos.y < SCREEN_HEIGHT - 150) {
        aiState.currentState = EnhancedAIState::NEUTRAL;
        aiState.stateTimer = 0;
    }
}

// Build a database of effective combos
void EnhancedEnemyAI::BuildComboDatabase() {
    // Create starter combo for low percent
    EnhancedAIState::ComboData lowCombo;
    lowCombo.sequence = {Character::UP_TILT, Character::UP_TILT, Character::UP_AIR};
    lowCombo.startingDamage = 0;
    lowCombo.isFinisher = false;
    lowCombo.hitstunRemaining = 0;
    aiState.knownCombos.push_back(lowCombo);

    // Create mid percent combo
    EnhancedAIState::ComboData midCombo;
    midCombo.sequence = {Character::DOWN_TILT, Character::FORWARD_AIR};
    midCombo.startingDamage = 40;
    midCombo.isFinisher = false;
    midCombo.hitstunRemaining = 0;
    aiState.knownCombos.push_back(midCombo);

    // Create kill combo for high percent
    EnhancedAIState::ComboData killCombo;
    killCombo.sequence = {Character::DOWN_THROW, Character::UP_AIR, Character::UP_SPECIAL};
    killCombo.startingDamage = 90;
    killCombo.isFinisher = true;
    killCombo.hitstunRemaining = 0;
    aiState.knownCombos.push_back(killCombo);

    // Edge guarding combo
    EnhancedAIState::ComboData edgeCombo;
    edgeCombo.sequence = {Character::BACK_AIR, Character::DOWN_AIR};
    edgeCombo.startingDamage = 60;
    edgeCombo.isFinisher = true;
    edgeCombo.hitstunRemaining = 0;
    aiState.knownCombos.push_back(edgeCombo);
}

// Adapt AI behavior based on difficulty setting
void EnhancedEnemyAI::AdaptToDifficulty() {
    // Set reaction times and decision making based on difficulty
    reactionTimeBase = 15.0f - (difficulty * 10.0f); // 5-15 frames reaction time
    reactionTimeVariance = 10.0f - (difficulty * 8.0f); // 2-10 frames variance

    // Decision delay scales with difficulty
    aiState.decisionDelay = static_cast<int>(10.0f - (difficulty * 7.0f)); // 3-10 frames to make decisions

    // Risk tolerance increases with difficulty
    aiState.riskTolerance = 0.3f + (difficulty * 0.5f); // 0.3-0.8 risk tolerance

    // Execution precision
    executionPrecision = 0.5f + (difficulty * 0.5f); // 0.5-1.0 precision

    // Decision quality
    decisionQuality = 0.6f + (difficulty * 0.4f); // 0.6-1.0 quality

    // At very low difficulties, make AI deliberately make mistakes
    if (difficulty < 0.3f) {
        // Sometimes forget to recover
        if (GetRandomValue(0, 100) < 20) {
            aiState.riskTolerance = 0.9f; // Very risky decisions
        }

        // Sometimes use wrong attacks
        if (GetRandomValue(0, 100) < 30) {
            executionPrecision = 0.2f;
        }
    }
}