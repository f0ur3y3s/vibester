// EnhancedAIState.h
#pragma once

#include "IAIState.h"
#include <deque>
#include <unordered_map>
#include <utility>
#include <memory>

class Character;
struct Vector2;

class EnhancedAIState : public IAIState {
public:
    // AI states definition
    enum State {
        NEUTRAL,
        APPROACH,
        ATTACK,
        PRESSURE,
        BAIT,
        DEFEND,
        PUNISH,
        RECOVER,
        RETREAT,
        EDGE_GUARD,
        LEDGE_TRAP,
        COMBO
    };

    // Combo data structure
    struct ComboData {
        std::vector<int> sequence;
        float startingDamage;
        bool isFinisher;
        int hitstunRemaining;
    };

    // Constructor
    EnhancedAIState();

    // IAIState interface implementation
    void UpdateState(Character* enemy, Character* player, int frameCount) override;
    void AnalyzePlayerPatterns() override;
    bool DetectPlayerHabit(const std::deque<CharacterState>& history,
                          CharacterState state,
                          float threshold) override;

    // State accessors
    State GetCurrentState() const { return currentState; }
    void SetCurrentState(State newState) {
        currentState = newState;
        stateTimer = 0;
    }

    float GetThreatLevel() const { return threatLevel; }
    float GetExpectedReward() const { return expectedReward; }

    bool IsOffStage() const { return isOffStage; }
    bool IsPlayerOffStage() const { return playerIsOffStage; }

    void SetOffStageStatus(bool enemyOffStage, bool playerOffStage) {
        isOffStage = enemyOffStage;
        playerIsOffStage = playerOffStage;
    }

    // Positional flags
    bool nearLeftEdge;
    bool nearRightEdge;
    bool abovePlayer;
    bool belowPlayer;

    // Combat state
    int lastAttackFrame;
    bool wasPlayerAttacking;
    bool comboState;
    int comboCounter;
    ComboData currentCombo;
    std::vector<ComboData> knownCombos;

    // Timer and frame tracking
    int stateTimer;
    int decisionDelay;
    int reactionTime;
    int adaptiveTimer;

    // Distance tracking
    float lastDistanceX;
    float lastDistanceY;

    // Advantage tracking
    float stockAdvantage;
    float damageAdvantage;

    // Risk/reward assessment
    float currentRiskLevel;
    float riskTolerance;
    float expectedReward;
    float centerControlImportance;

    // Threat assessment
    float threatLevel;

    // Update player behavior profiles
    void UpdatePlayerBehaviorProfiles();

    // Player pattern analysis
    float playerAggressionLevel;
    float playerDefenseLevel;
    float playerRecoveryPattern;
    float playerEdgeHabit;

    // Frequency tracking
    std::unordered_map<int, int> playerAttackFrequency;

    // Player habit analysis
    bool playerFavorsGround;
    bool playerFavorsAerial;
    bool playerShieldsOften;
    bool playerRollsOften;
    bool playerJumpsOutOfCombos;

    // History tracking
    std::deque<int> lastPlayerAttacks;
    std::deque<std::pair<Vector2, int>> playerPositionHistory;
    std::deque<CharacterState> playerStateHistory;

private:
    State currentState;
    bool isOffStage;
    bool playerIsOffStage;
};
