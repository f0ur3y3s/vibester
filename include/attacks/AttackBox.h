#ifndef ATTACK_BOX_H
#define ATTACK_BOX_H

#include "raylib.h"

// Hitbox for character attacks
class AttackBox {
public:
    enum HitboxType {
        NORMAL,
        GRAB,
        PROJECTILE
    };
    
    Rectangle rect;
    float damage;
    float baseKnockback;
    float knockbackScaling;
    float knockbackAngle;
    int hitLag;
    int shieldStun;
    HitboxType type;
    bool isActive;
    
    AttackBox(
        Rectangle r, 
        float dmg, 
        float baseKb, 
        float kbScaling, 
        float kbAngle, 
        int lag, 
        int shield
    );
    
    void update();
    void draw(bool debug = false);
};

#endif // ATTACK_BOX_H