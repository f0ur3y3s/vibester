#ifndef CHARACTER_H
#define CHARACTER_H

#include "raylib.h"
#include "Platform.h"
#include "AttackBox.h"
#include "Constants.h"
#include <string>
#include <vector>

// Character class
class Character {
public:
    Vector2 position;
    Vector2 velocity;
    float width;
    float height;
    float speed;
    bool isJumping;
    bool isFacingRight;
    bool isAttacking;
    int damage;
    Color color;
    std::string name;
    std::vector<AttackBox> attacks;
    
    // Animation variables
    int currentFrame;
    int framesCounter;
    int framesSpeed;
    bool isSpecialAttack;
    
    // Attack variables
    enum AttackType {
        NONE,
        NEUTRAL,
        SIDE,
        UP,
        DOWN,
        SPECIAL_NEUTRAL,
        SPECIAL_SIDE,
        SPECIAL_UP,
        SPECIAL_DOWN
    };
    
    AttackType currentAttack;
    int attackDuration;
    int attackFrame;
    bool canAttack;
    
    // Cooldowns
    int specialCooldown;
    int currentCooldown;
    int specialSideCooldown;
    int currentSideCooldown;
    int specialUpCooldown;
    int currentUpCooldown;
    int specialDownCooldown;
    int currentDownCooldown;
    
    // For recovery move
    bool isRecovering;
    
    // Death animation
    bool isDying;
    float deathRotation;
    float deathScale;
    int deathDuration;
    int deathFrame;
    Vector2 deathVelocity;
    Vector2 deathPosition;
    
    Character(float x, float y, float w, float h, float spd, Color col, std::string n);
    
    Rectangle getRect();
    void update(std::vector<Platform>& platforms);
    void updateAttackPositions();
    void startDeathAnimation();
    void updateDeathAnimation();
    void resetAttackState();
    void draw();
    void drawDeathAnimation();
    
    // Movement methods
    void jump();
    void moveLeft();
    void moveRight();
    
    // Standard attacks
    void neutralAttack();
    void sideAttack();
    void upAttack();
    void downAttack();
    
    // Special attacks
    void specialNeutralAttack();
    void specialSideAttack();
    void specialUpAttack();
    void specialDownAttack();
    
    bool checkHit(Character& other);
};

#endif // CHARACTER_H