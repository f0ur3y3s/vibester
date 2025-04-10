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
    int duration;          // How long hitbox is active
    int currentFrame;      // Current frame counter
    
    // For projectiles
    Vector2 velocity;      // Movement speed and direction
    bool destroyOnHit;     // Whether projectile is destroyed on hit
    
    // Standard constructor
    AttackBox(
        Rectangle r, 
        float dmg, 
        float baseKb, 
        float kbScaling, 
        float kbAngle, 
        int lag, 
        int shield
    );
    
    // Constructor for projectiles
    AttackBox(
        Rectangle r, 
        float dmg, 
        float baseKb, 
        float kbScaling, 
        float kbAngle, 
        int lag, 
        int dur,
        Vector2 vel, 
        bool destroy = true
    );
    
    bool update();  // Returns false if attack is expired
    void draw(bool debug = false);
};

#endif // ATTACK_BOX_H