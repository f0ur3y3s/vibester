#ifndef CHARACTER_H
#define CHARACTER_H

#include "raylib.h"
#include "Platform.h"
#include "AttackBox.h"
#include "Particle.h"
#include "PhysicsState.h"
#include "StateManager.h"
#include "CharacterConfig.h"
#include <string>
#include <vector>

// Forward declaration
class Character;

// Hit effect struct
struct HitEffect
{
    Vector2 position;
    Color color;
    int duration;
    int currentFrame;
    float size;

    HitEffect(Vector2 pos, Color col);
    bool update();
    void draw();
};

// Character class - enhanced for Smash Bros style
class Character
{
public:
    // Basic properties
    float width;
    float height;
    float speed;
    Color color;
    std::string name;

    // Core systems
    PhysicsState physics; // Handles movement and physics
    StateManager stateManager; // Handles state and attack management

    // Smash-style properties
    float damagePercent; // Damage as percentage (0-999%)
    int stocks; // Lives remaining

    // Visual elements
    int currentFrame; // Animation tracking
    int framesCounter;
    int framesSpeed;

    // Death animation properties
    float deathRotation;
    float deathScale;
    Vector2 deathVelocity;
    Vector2 deathPosition;

    // Grab reference
    Character* grabbedCharacter;

    // Visual effects
    std::vector<HitEffect> hitEffects;
    std::vector<AttackBox> attacks;

    // Explosion state
    std::vector<Particle> explosionParticles;

    // Constructor
    Character(float x, float y, float w, float h, float spd, Color col, std::string n);

    // Accessors (for future transition to encapsulation)
    float getDamagePercent() const;
    int getStocks() const;
    std::string getName() const;

    void checkForExplosion();
    void startExplosionAnimation();
    void updateExplosionAnimation();
    void drawExplosionAnimation();

    // Basic methods
    Rectangle getRect();
    Rectangle getHurtbox(); // Possibly smaller than character rect
    void update(std::vector<Platform>& platforms);
    void updateAttackPositions();
    void draw();

    // Death handling
    void startDeathAnimation();
    void updateDeathAnimation();
    void drawDeathAnimation();
    void respawn(Vector2 spawnPoint);

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
