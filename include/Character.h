#ifndef CHARACTER_H
#define CHARACTER_H

#include <Particle.h>

#include "raylib.h"
#include "Platform.h"
#include "AttackBox.h"
#include "Constants.h"
#include <string>
#include <vector>

// Forward declaration
class Character;

// Hit effect struct
struct HitEffect {
    Vector2 position;
    int duration;
    int currentFrame;
    float size;
    Color color;

    HitEffect(Vector2 pos, Color col);
    bool update();
    void draw();
};

// Character class - enhanced for Smash Bros style
class Character {
public:
    // Basic properties
    Vector2 position;
    Vector2 velocity;
    float width;
    float height;
    float speed;
    Color color;
    std::string name;

    // Smash-style properties
    float damagePercent;    // Damage as percentage (0-999%)
    int stocks;             // Lives remaining
    bool isInvincible;      // Invincibility frames
    int invincibilityFrames;

    // State flags
    bool isFacingRight;
    bool isJumping;
    bool hasDoubleJump;     // Smash-style double jump
    bool isAttacking;
    bool isShielding;       // Shield mechanic
    float shieldHealth;     // Shield breaks if too low
    bool isDodging;         // Dodge roll/spotdodge
    int dodgeFrames;
    bool isFastFalling;     // Fast fall mechanic

    // Animation variables
    int currentFrame;
    int framesCounter;
    int framesSpeed;
    bool isHitstun;         // Taking knockback
    int hitstunFrames;

    // Attack system
    enum CharacterState {
        IDLE,
        RUNNING,
        JUMPING,
        FALLING,
        ATTACKING,
        SHIELDING,
        DODGING,
        HITSTUN,
        DYING
    };

    CharacterState state;

    // Attack types - expanded Smash style
    enum AttackType {
        NONE,
        // Ground attacks
        JAB,                // A
        FORWARD_TILT,       // Side + A
        UP_TILT,            // Up + A
        DOWN_TILT,          // Down + A
        DASH_ATTACK,        // Run + A

        // Smash attacks
        FORWARD_SMASH,      // Side + A (charged)
        UP_SMASH,           // Up + A (charged)
        DOWN_SMASH,         // Down + A (charged)

        // Aerial attacks
        NEUTRAL_AIR,        // A in air
        FORWARD_AIR,        // Forward + A in air
        BACK_AIR,           // Back + A in air
        UP_AIR,             // Up + A in air
        DOWN_AIR,           // Down + A in air

        // Special attacks
        NEUTRAL_SPECIAL,    // B
        SIDE_SPECIAL,       // Side + B
        UP_SPECIAL,         // Up + B (recovery)
        DOWN_SPECIAL,       // Down + B

        // Grabs and throws
        GRAB,               // Z or Shield + A
        PUMMEL,             // A while grabbing
        FORWARD_THROW,      // Forward while grabbing
        BACK_THROW,         // Back while grabbing
        UP_THROW,           // Up while grabbing
        DOWN_THROW          // Down while grabbing
    };

    AttackType currentAttack;
    int attackDuration;
    int attackFrame;
    bool canAttack;
    std::vector<AttackBox> attacks;

    // Grab state
    bool isGrabbing;
    Character* grabbedCharacter;
    int grabDuration;
    int grabFrame;

    // Cooldowns
    struct Cooldown {
        int duration;
        int current;
    };

    Cooldown specialNeutralCD;
    Cooldown specialSideCD;
    Cooldown specialUpCD;
    Cooldown specialDownCD;
    Cooldown dodgeCD;

    // Death animation
    bool isDying;
    float deathRotation;
    float deathScale;
    int deathDuration;
    int deathFrame;
    Vector2 deathVelocity;
    Vector2 deathPosition;

    // Visual effects
    std::vector<HitEffect> hitEffects;

    // Constructor
    Character(float x, float y, float w, float h, float spd, Color col, std::string n);

    void checkForExplosion();

    void startExplosionAnimation();

    void updateExplosionAnimation();

    void drawExplosionAnimation();

    // Basic methods
    Rectangle getRect();
    Rectangle getHurtbox();  // Possibly smaller than character rect
    void update(std::vector<Platform>& platforms);
    void updateAttackPositions();
    void updateCooldowns();
    void draw();

    // Death handling
    void startDeathAnimation();
    void updateDeathAnimation();
    void drawDeathAnimation();
    void respawn(Vector2 spawnPoint);
    bool isExploding;
    int explosionFrame;
    int explosionDuration;
    std::vector<Particle> explosionParticles;

    // State management
    void changeState(CharacterState newState);
    void resetAttackState();

    // Movement methods
    void jump();
    void doubleJump();
    void moveLeft();
    void moveRight();
    void fastFall();
    void dropThroughPlatform();

    // Defense methods
    void shield();
    void releaseShield();
    void spotDodge();
    void forwardDodge();
    void backDodge();
    void airDodge(float dirX, float dirY);


    // Standard ground attacks
    void jab();
    void forwardTilt();
    void upTilt();
    void downTilt();
    void dashAttack();
    
    // Simple attack wrappers for main.cpp
    void neutralAttack() { jab(); }
    void sideAttack() { forwardTilt(); }
    void upAttack() { upTilt(); }
    void downAttack() { downTilt(); }
    void specialNeutralAttack() { neutralSpecial(); }
    void specialSideAttack() { sideSpecial(); }
    void specialUpAttack() { upSpecial(); }
    void specialDownAttack() { downSpecial(); }

    // Smash attacks
    void forwardSmash(float chargeTime);
    void upSmash(float chargeTime);
    void downSmash(float chargeTime);

    // Aerial attacks
    void neutralAir();
    void forwardAir();
    void backAir();
    void upAir();
    void downAir();

    // Special attacks
    void neutralSpecial();
    void sideSpecial();
    void upSpecial();
    void downSpecial();

    // Grab and throws
    void grab();
    void pummel();
    void forwardThrow();
    void backThrow();
    void upThrow();
    void downThrow();
    void releaseGrab();

    // Collision and damage
    bool checkHit(Character& other);
    void applyDamage(float damage);
    void applyKnockback(float damage, float baseKnockback, float knockbackScaling, float directionX, float directionY);
    void createHitEffect(Vector2 position);
    bool isOutOfBounds();
};

#endif // CHARACTER_H