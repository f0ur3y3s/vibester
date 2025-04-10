// EnhancedAIController.h
#pragma once

#include "IAIController.h"
#include "EnhancedAIState.h"
#include "AIDecisionMaker.h"
#include "AIExecutor.h"
#include "AIConfig.h"
#include <memory>

class EnhancedAIController : public IAIController {
public:
    EnhancedAIController();
    virtual ~EnhancedAIController() = default;

    // IAIController interface implementation
    void Update(std::vector<Character*>& players, std::vector<Platform>& platforms) override;
    void SetDifficulty(float difficulty) override;
    float GetDifficulty() const override;

    // Additional methods for the Enhanced AI
    EnhancedAIState::State GetCurrentState() const;
    float GetCurrentConfidence() const;

private:
    // Core AI components
    std::unique_ptr<EnhancedAIState> aiState;
    std::unique_ptr<AIDecisionMaker> decisionMaker;
    std::unique_ptr<AIExecutor> executor;
    AIConfig config;

    // Tracking variables
    int frameCount;
    bool wasComboEffective;
    bool shouldFeint;
    float lastDIEffectiveness;

    // Helper methods
    void ExecuteComboBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    bool IsOffStage(Vector2 position, const std::vector<Platform>& platforms);
};
