#ifndef ATTACK_FACTORY_H
#define ATTACK_FACTORY_H

#include "attacks/AttackBox.h"
#include <vector>

// Forward declaration to avoid circular dependency
class Character;

class AttackFactory {
public:
    AttackFactory(Character& character) : character(character) {}
    
    // Ground attacks
    std::vector<AttackBox> createJab();
    std::vector<AttackBox> createForwardTilt();
    std::vector<AttackBox> createUpTilt();
    std::vector<AttackBox> createDownTilt();
    std::vector<AttackBox> createDashAttack();
    
    // Smash attacks
    std::vector<AttackBox> createForwardSmash(float chargeTime);
    std::vector<AttackBox> createUpSmash(float chargeTime);
    std::vector<AttackBox> createDownSmash(float chargeTime);
    
    // Aerial attacks
    std::vector<AttackBox> createNeutralAir();
    std::vector<AttackBox> createForwardAir();
    std::vector<AttackBox> createBackAir();
    std::vector<AttackBox> createUpAir();
    std::vector<AttackBox> createDownAir();
    
    // Special attacks
    std::vector<AttackBox> createNeutralSpecial();
    std::vector<AttackBox> createSideSpecial();
    std::vector<AttackBox> createUpSpecial();
    std::vector<AttackBox> createDownSpecial();
    
    // Grab
    std::vector<AttackBox> createGrab();
    
    // Helper methods
    Rectangle createHitboxRect(float widthScale, float heightScale, float xOffset, float yOffset);
    
private:
    Character& character;
};

#endif // ATTACK_FACTORY_H