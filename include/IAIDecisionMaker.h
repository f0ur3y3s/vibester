// IAIDecisionMaker.h
#pragma once

#include <vector>

class Character;
class Platform;
class IAIState;

class IAIDecisionMaker {
public:
    virtual ~IAIDecisionMaker() = default;

    virtual void DetermineNextAction(std::vector<Character*>& players,
                                     const std::vector<Platform>& platforms,
                                     IAIState& state) = 0;

    virtual float AssessRisk(Character* enemy, Character* player, int stateId) = 0;
    virtual float PredictReward(Character* enemy, Character* player, int stateId) = 0;
};
