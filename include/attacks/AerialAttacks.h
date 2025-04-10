#ifndef AERIAL_ATTACKS_H
#define AERIAL_ATTACKS_H

#include "../character/Character.h"

namespace AerialAttacks {
    void executeNeutralAir(Character* character);
    void executeForwardAir(Character* character);
    void executeBackAir(Character* character);
    void executeUpAir(Character* character);
    void executeDownAir(Character* character);
}

#endif // AERIAL_ATTACKS_H