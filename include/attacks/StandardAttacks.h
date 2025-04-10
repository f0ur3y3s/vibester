#ifndef STANDARD_ATTACKS_H
#define STANDARD_ATTACKS_H

#include "../character/Character.h"

namespace StandardAttacks {
    // Ground attacks
    void executeJab(Character* character);
    void executeForwardTilt(Character* character);
    void executeUpTilt(Character* character);
    void executeDownTilt(Character* character);
    void executeDashAttack(Character* character);
    
    // Smash attacks
    void executeForwardSmash(Character* character, float chargeTime);
    void executeUpSmash(Character* character, float chargeTime);
    void executeDownSmash(Character* character, float chargeTime);
}

#endif // STANDARD_ATTACKS_H