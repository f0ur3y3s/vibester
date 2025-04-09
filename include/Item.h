#ifndef ITEM_H
#define ITEM_H

#include "raylib.h"
#include "Constants.h"
#include "AttackBox.h"
#include <string>
#include <vector>

// Forward declaration
class Character;

// Item class for Smash Bros style items
class Item {
public:
    enum ItemType {
        CONTAINER,      // Crates, barrels, etc.
        BATTERING,      // Beam sword, baseball bat, etc.
        SHOOTING,       // Super scope, ray gun, etc.
        THROWING,       // Red/green shells, bob-ombs, etc.
        RECOVERY,       // Food, heart container, etc.
        SPECIAL,        // Smash ball, etc.
        SUPPORT         // Poké ball, assist trophy, etc.
    };

    // Basic properties
    Vector2 position;
    Vector2 velocity;
    float width;
    float height;
    ItemType type;
    std::string name;

    // State flags
    bool isOnGround;
    bool isBroken;
    bool isActive;
    bool isHeld;          // Currently being held by a character
    Character* holder;    // Which character is holding this item

    // Physics
    float gravity;
    float friction;
    float bounceAmount;   // How much it bounces when hitting ground

    // Durability (for battering items)
    int durability;
    int maxUses;

    // Animation variables
    int currentFrame;
    int framesCounter;
    int framesSpeed;

    // For shooting/throwing items
    std::vector<AttackBox> projectiles;
    int ammo;             // For shooting items
    int cooldown;
    int currentCooldown;

    // Visual
    Color color;
    bool isFlashing;      // Flashing when about to disappear
    int flashCounter;
    int lifespan;         // How long it exists before disappearing
    int currentLife;

    // Constructor
    Item(ItemType type, float x, float y, std::string name);

    // Core methods
    Rectangle getRect();
    void update(std::vector<Platform>& platforms);
    void draw();

    // Item usage
    void pickup(Character* character);
    void drop(float velocityX, float velocityY);
    void throw_(float velocityX, float velocityY);
    void use();         // Primary usage (A button)
    void specialUse();  // Secondary usage (Special moves while holding)

    // Effect methods (used by different item types)
    void damageEffect(Character* target, float amount);
    void healEffect(Character* target, float amount);
    void explosionEffect(Vector2 center, float radius, float damage, float knockback);
    void spawnProjectile(Vector2 position, Vector2 velocity, float damage);
    void breakItem();    // For containers

    // Each item type will implement these differently
    virtual void onPickup();
    virtual void onDrop();
    virtual void onUse();
    virtual void onSpecialUse();
    virtual void onBreak();
    virtual void onCollision(Character* character);

    // Factory method to create specific items
    static Item* createItem(const std::string& itemName, float x, float y);
};

// Specific item types
class BatteringItem : public Item {
public:
    BatteringItem(float x, float y, std::string name);
    void onUse() override;
    void drawHeld(Character* holder);

    AttackBox getHitbox(bool isFacingRight);
    float damageMultiplier;
    float knockbackMultiplier;
    bool canSmash;         // Can be charged like a smash attack
};

class ThrowingItem : public Item {
public:
    ThrowingItem(float x, float y, std::string name);
    void onUse() override;
    void onCollision(Character* character) override;

    float throwSpeed;
    bool explodes;
    float explosionRadius;
    float explosionDamage;
};

class ShootingItem : public Item {
public:
    ShootingItem(float x, float y, std::string name);
    void onUse() override;
    void onSpecialUse() override;

    float projectileSpeed;
    float projectileDamage;
    int fireRate;          // How many frames between shots
};

class ContainerItem : public Item {
public:
    ContainerItem(float x, float y, std::string name);
    void onBreak() override;

    std::vector<std::string> possibleContents;
    int contentCount;
};

class RecoveryItem : public Item {
public:
    RecoveryItem(float x, float y, std::string name);
    void onCollision(Character* character) override;
    void onUse() override;

    float healAmount;
};

#endif // ITEM_H