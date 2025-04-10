// IAIExecutor.h
#pragma once

class Character;

class IAIExecutor {
public:
    virtual ~IAIExecutor() = default;

    virtual void ExecuteAction(Character* enemy, Character* player,
                              float distanceX, float distanceY, int actionId) = 0;
};
