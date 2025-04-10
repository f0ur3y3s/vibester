#ifndef CHARACTER_MOVEMENT_H
#define CHARACTER_MOVEMENT_H

#include "Character.h"

namespace CharacterMovement {
    // Basic movement
    void executeJump(Character* character);
    void executeDoubleJump(Character* character);
    void executeMoveLeft(Character* character);
    void executeMoveRight(Character* character);
    void executeFastFall(Character* character);
    void executeDropThroughPlatform(Character* character);
    
    // Defensive movement
    void executeShield(Character* character);
    void executeReleaseShield(Character* character);
    void executeSpotDodge(Character* character);
    void executeForwardDodge(Character* character);
    void executeBackDodge(Character* character);
    void executeAirDodge(Character* character, float dirX, float dirY);
}

#endif // CHARACTER_MOVEMENT_H