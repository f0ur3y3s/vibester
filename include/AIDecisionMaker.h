// AIDecisionMaker.h
#pragma once

#include "IAIDecisionMaker.h"
#include "EnhancedAIState.h"
#include "AIConfig.h"
#include <vector>
#include <string>

struct ZoneStrategy {
    Rectangle zone;
    EnhancedAIState::State preferredState;
    std::vector<int> preferredAttacks;
    float priorityMultiplier;
};

class AIDecisionMaker : public IAIDecisionMaker {
public:
    AIDecisionMaker(AIConfig& config);

    // IAIDecisionMaker interface implementation
    void DetermineNextAction(std::vector<Character*>& players,
                            const std::vector<Platform>& platforms,
                            IAIState& state) override;

    float AssessRisk(Character* enemy, Character* player, int stateId) override;
    float PredictReward(Character* enemy, Character* player, int stateId) override;

private:
    // Helper methods for decision making
    void UpdateThreatLevel(EnhancedAIState& aiState, Character* player, float absDistanceX, float absDistanceY);
    void UpdateZoneAwareness(EnhancedAIState& aiState, Character* enemy, Character* player, const std::vector<Platform>& platforms);
    bool IsOffStage(Vector2 position, const std::vector<Platform>& platforms);
    EnhancedAIState::State ChooseBestState(std::vector<std::pair<EnhancedAIState::State, float>>& stateOptions);
    bool AttemptCombo(EnhancedAIState& aiState, Character* enemy, Character* player);
    void BuildComboDatabase(EnhancedAIState& aiState);

    // Configuration
    AIConfig& config;

    // Zone-based strategy
    struct ZoneStrategy {
        Rectangle zone;
        EnhancedAIState::State preferredState;
        std::vector<int> preferredAttacks;
        float priorityMultiplier;
    };

    std::vector<ZoneStrategy> zoneStrategies;
};
