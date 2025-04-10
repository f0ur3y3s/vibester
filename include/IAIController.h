// IAIController.h
#pragma once

#include <vector>
class Character;
class Platform;

class IAIController {
public:
    virtual ~IAIController() = default;

    virtual void Update(std::vector<Character*>& players, std::vector<Platform>& platforms) = 0;
    virtual void SetDifficulty(float difficulty) = 0;
    virtual float GetDifficulty() const = 0;
};
