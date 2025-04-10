#ifndef ENHANCED_ENEMY_AI_H
#define ENHANCED_ENEMY_AI_H

#include "raylib.h"
#include "Character.h"
#include "Constants.h"
#include <vector>
#include <deque>
#include <array>
#include <memory>

// Forward declarations
class AttackOption;

// Enhanced AI state machine parameters
struct EnhancedAIState {
    enum State {
        NEUTRAL,            // Waiting and evaluating
        APPROACH,           // Move toward player
        ATTACK,             // Execute attacks when in range
        PRESSURE,           // Keep attacking to maintain advantage
        BAIT,               // Movement to bait player attacks
        DEFEND,             // Shield or dodge when player attacks
        PUNISH,             // Execute punish after successful defense
        RECOVER,            // Return to stage when off-stage
        RETREAT,            // Back away to reset neutral game
        EDGE_GUARD,         // Attempt to prevent player recovery
        LEDGE_TRAP,         // Trap player near ledge
        COMBO               // Execute combos
    };

    State currentState;
    int stateTimer;
    int decisionDelay;
    Vector2 targetPosition;
    int reactionTime;
    int lastAttackFrame;
    bool wasPlayerAttacking;
    float threatLevel;
    bool isOffStage;
    bool playerIsOffStage;
    float lastDistanceX;
    float lastDistanceY;
    int adaptiveTimer;
    bool comboState;
    int comboCounter;

    // Positional awareness
    bool nearLeftEdge;
    bool nearRightEdge;
    bool abovePlayer;
    bool belowPlayer;

    // Action history for pattern recognition
    std::deque<Character::AttackType> lastPlayerAttacks;
    std::deque<std::pair<Vector2, int>> playerPositionHistory; // Position and frame
    std::deque<Character::CharacterState> playerStateHistory;

    // Adaptation variables
    float playerAggressionLevel;
    float playerDefenseLevel;
    float playerRecoveryPattern;
    float playerEdgeHabit;
    int playerAttackFrequency[Character::DOWN_THROW + 1]; // Count of each attack type

    // Player habit analysis
    bool playerFavorsGround;
    bool playerFavorsAerial;
    bool playerShieldsOften;
    bool playerRollsOften;
    bool playerJumpsOutOfCombos;

    // Risk assessment
    float currentRiskLevel;      // 0.0 = safe, 1.0 = dangerous
    float riskTolerance;         // How much risk AI is willing to take (adjusts dynamically)
    float expectedReward;        // Potential payoff for current action

    // Stage control
    float centerControlImportance;  // Value of controlling center stage

    // Combo system
    struct ComboData {
        std::vector<Character::AttackType> sequence;
        float startingDamage;
        bool isFinisher;
        int hitstunRemaining;
    };

    ComboData currentCombo;
    std::vector<ComboData> knownCombos;

    // Match awareness
    float stockAdvantage;      // +1 = AI has 1 more stock than player
    float damageAdvantage;     // How much AI's damage differs from player

    EnhancedAIState();
    void updateHistory(Character* player, int frameCount);
    void analyzePlayerPatterns();
    bool detectPlayerHabit(const std::deque<Character::CharacterState>& history, Character::CharacterState state, float threshold);
};

class EnhancedEnemyAI {
public:
    // Constructor
    EnhancedEnemyAI();

    // Main update function to be called from Game.cpp
    void Update(std::vector<Character*>& players, std::vector<Platform>& platforms);

    // Get current AI state (for debug display)
    EnhancedAIState::State GetCurrentState() const;

    // Get confidence level in current strategy (for debug)
    float GetCurrentConfidence() const;

    // Set difficulty level (0.0 = easiest, 1.0 = hardest)
    void SetDifficulty(float difficulty);

private:
    // AI state
    EnhancedAIState aiState;
    float difficulty;          // 0.0 to 1.0 scale
    int frameCount;            // Track frames for timing

    // Situation awareness
    float lastDIEffectiveness;  // How well DI worked last time
    bool wasComboEffective;
    bool shouldFeint;

    // Dynamic difficulty adjustments
    float reactionTimeBase;
    float reactionTimeVariance;
    float decisionQuality;
    float executionPrecision;

    // Positional strategies
    struct ZoneStrategy {
        Rectangle zone;
        EnhancedAIState::State preferredState;
        std::vector<Character::AttackType> preferredAttacks;
        float priorityMultiplier;
    };
    std::vector<ZoneStrategy> zoneStrategies;

    // Attack options management
    std::vector<std::unique_ptr<AttackOption>> attackOptions;
    void initializeAttackOptions();

    // Helper functions
    bool IsOffStage(Vector2 position, const std::vector<Platform>& platforms);
    void UpdateThreatLevel(Character* player, float absDistanceX, float absDistanceY);
    void DetermineAIState(float absDistanceX, float absDistanceY, std::vector<Character*>& players, const std::vector<Platform>& platforms);
    void UpdateZoneAwareness(Character* enemy, Character* player, const std::vector<Platform>& platforms);

    // State-specific behaviors with enhanced decision making
    void ExecuteNeutralBehavior(Character* enemy, Character* player);
    void ExecuteApproachBehavior(float distanceX, float distanceY, float absDistanceX, float absDistanceY, Character* enemy);
    void ExecuteAttackBehavior(float distanceX, float distanceY, float absDistanceX, float absDistanceY, Character* enemy, Character* player);
    void ExecutePressureBehavior(float distanceX, float distanceY, Character* enemy, Character* player);
    void ExecuteBaitBehavior(float distanceX, float distanceY, Character* enemy, Character* player);
    void ExecuteDefendBehavior(float distanceX, float distanceY, Character* enemy, Character* player);
    void ExecutePunishBehavior(float distanceX, float distanceY, Character* enemy, Character* player);
    void ExecuteRecoverBehavior(float distanceX, float absDistanceX, Character* enemy, const std::vector<Platform>& platforms);
    void ExecuteRetreatBehavior(float distanceX, Character* enemy, Character* player);
    void ExecuteEdgeGuardBehavior(Vector2 playerPos, Vector2 enemyPos, Character* enemy, Character* player);
    void ExecuteLedgeTrapBehavior(Vector2 playerPos, Vector2 enemyPos, Character* enemy, Character* player);
    void ExecuteComboBehavior(float distanceX, float distanceY, Character* enemy, Character* player);

    // Combat functions
    bool AttemptCombo(Character* enemy, Character* player);
    Character::AttackType ChooseBestAttack(Character* enemy, Character* player, float distanceX, float distanceY);
    bool ShouldShield(Character* enemy, Character* player);
    bool ShouldDodge(Character* enemy, Character* player, float& directionX, float& directionY);
    void ApplyDirectionalInfluence(Character* enemy);

    // Adaptation functions
    void AdaptToDifficulty();
    void UpdatePlayerProfile(Character* player);
    void BuildComboDatabase();
    bool RecognizePlayerPattern();
    float EvaluateOptionUtility(Character::AttackType attackType, float distanceX, float distanceY, Character* enemy, Character* player);

    // Smart recovery functions
    void PlanRecoveryPath(Character* enemy, const std::vector<Platform>& platforms);
    bool IsPathToStageClear(Vector2 start, Vector2 end, Character* player);
    float CalculateRecoveryAngle(Character* enemy, const std::vector<Platform>& platforms);

    // Risk assessment
    float AssessRisk(Character* enemy, Character* player, EnhancedAIState::State potentialState);
    float PredictReward(Character* enemy, Character* player, EnhancedAIState::State potentialState);
};

// Abstraction for attack options with utilities
class AttackOption {
public:
    virtual ~AttackOption() {}
    virtual float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) = 0;
    virtual void Execute(Character* enemy) = 0;
    virtual Character::AttackType GetAttackType() const = 0;
    virtual bool IsViable(float distanceX, float distanceY, Character* enemy) = 0;
};

// Concrete implementations for different attack options
class JabAttack : public AttackOption {
public:
    float GetUtility(float distanceX, float distanceY, Character* enemy, Character* player) override;
    void Execute(Character* enemy) override;
    Character::AttackType GetAttackType() const override { return Character::JAB; }
    bool IsViable(float distanceX, float distanceY, Character* enemy) override;
};

// More attack option classes would be defined similarly

#endif // ENHANCED_ENEMY_AI_H