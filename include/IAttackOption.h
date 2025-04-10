// IAttackOption.h
#pragma once

class Character;

class IAttackOption {
public:
    virtual ~IAttackOption() = default;

    virtual float GetUtility(float distanceX, float distanceY,
                            Character* enemy, Character* player) = 0;
    virtual void Execute(Character* enemy) = 0;
    virtual bool IsViable(float distanceX, float distanceY, Character* enemy) = 0;
    virtual int GetAttackType() const = 0;
};
