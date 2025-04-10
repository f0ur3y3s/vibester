// AIExecutor.h
#pragma once

#include "IAIExecutor.h"
#include "EnhancedAIState.h"
#include "IAttackOption.h"
#include "AIConfig.h"
#include "Platform.h"
#include <vector>
#include <memory>

class AIExecutor : public IAIExecutor {
public:
    AIExecutor(AIConfig& config);

    // IAIExecutor interface implementation
    void ExecuteAction(Character* enemy, Character* player,
                      float distanceX, float distanceY, int actionId) override;

    // Specialized behavior execution methods
    void ExecuteNeutralBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    void ExecuteApproachBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    void ExecuteAttackBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    void ExecutePressureBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    void ExecuteBaitBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    void ExecuteDefendBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    void ExecutePunishBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    void ExecuteRecoverBehavior(Character* enemy, const std::vector<Platform>& platforms, float distanceX, float distanceY);
    void ExecuteRetreatBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    void ExecuteEdgeGuardBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    void ExecuteLedgeTrapBehavior(Character* enemy, Character* player, float distanceX, float distanceY);
    void ExecuteComboBehavior(EnhancedAIState& aiState, Character* enemy, Character* player, float distanceX, float distanceY);

    // Helper methods
    int ChooseBestAttack(Character* enemy, Character* player, float distanceX, float distanceY);
    float CalculateRecoveryAngle(Character* enemy, const std::vector<Platform>& platforms);
    void ApplyDirectionalInfluence(Character* enemy);

    // Set platform reference for recovery
    void SetPlatforms(const std::vector<Platform>* platforms) {
        this->platforms = platforms;
    }

private:
    AIConfig& config;
    std::vector<std::unique_ptr<IAttackOption>> attackOptions;
    const std::vector<Platform>* platforms;
};