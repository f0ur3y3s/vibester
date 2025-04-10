#include "AttackFactory.h"
#include "Character.h"
#include <cmath>

// Helper method to create hitbox rectangles relative to character size
Rectangle AttackFactory::createHitboxRect(float widthScale, float heightScale, float xOffset, float yOffset) {
    float hitboxWidth = character.width * widthScale;
    float hitboxHeight = character.height * heightScale;

    // X position depends on facing direction
    float hitboxX;
    if (xOffset == 0) {
        // Center-aligned hitbox
        hitboxX = character.physics.position.x - hitboxWidth / 2;
    } else {
        // Offset hitbox (positive = in front, negative = behind)
        float offsetMult = character.stateManager.isFacingRight ? 1.0f : -1.0f;
        hitboxX = character.physics.position.x + (character.width / 2 * offsetMult * xOffset);

        // Adjust if offset is negative and character is facing left (or vice versa)
        if (!character.stateManager.isFacingRight) {
            hitboxX -= hitboxWidth;
        }
    }

    // Y position (positive = down, negative = up)
    float hitboxY = character.physics.position.y + (character.height / 2 * yOffset) - hitboxHeight / 2;

    return {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
}

// Ground attacks
std::vector<AttackBox> AttackFactory::createJab() {
    std::vector<AttackBox> attacks;

    // Small hitbox with minimal knockback
    Rectangle hitboxRect = createHitboxRect(0.7f, 0.5f, 1.0f, 0.0f);
    attacks.push_back(AttackBox(hitboxRect, 3.0f, 1.5f, 0.05f,
                               character.stateManager.isFacingRight ? 0.0f : 180.0f, 5, 5));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createForwardTilt() {
    std::vector<AttackBox> attacks;

    // Good range hitbox
    Rectangle hitboxRect = createHitboxRect(1.2f, 0.6f, 1.0f, 0.0f);
    attacks.push_back(AttackBox(hitboxRect, 8.0f, 4.0f, 0.15f,
                               character.stateManager.isFacingRight ? 30.0f : 150.0f, 15, 12));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createUpTilt() {
    std::vector<AttackBox> attacks;

    // Tall, narrow hitbox above character
    Rectangle hitboxRect = createHitboxRect(0.7f, 1.3f, 0.0f, -1.0f);
    attacks.push_back(AttackBox(hitboxRect, 7.0f, 3.0f, 0.15f, 80.0f, 12, 12));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createDownTilt() {
    std::vector<AttackBox> attacks;

    // Low-profile hitbox
    Rectangle hitboxRect = createHitboxRect(1.3f, 0.3f, 1.0f, 0.8f);
    attacks.push_back(AttackBox(hitboxRect, 5.0f, 2.5f, 0.1f,
                               character.stateManager.isFacingRight ? 15.0f : 165.0f, 10, 8));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createDashAttack() {
    std::vector<AttackBox> attacks;

    // Forward-moving hitbox
    Rectangle hitboxRect = createHitboxRect(1.1f, 0.8f, 1.0f, 0.0f);
    attacks.push_back(AttackBox(hitboxRect, 10.0f, 5.0f, 0.15f,
                               character.stateManager.isFacingRight ? 40.0f : 140.0f, 20, 20));

    return attacks;
}

// Smash attacks
std::vector<AttackBox> AttackFactory::createForwardSmash(float chargeTime) {
    std::vector<AttackBox> attacks;

    // Charge multiplier (1.0 to 1.5)
    float chargeMultiplier = 1.0f + std::min(chargeTime / 60.0f, 0.5f);

    // Large, strong forward hitbox
    Rectangle hitboxRect = createHitboxRect(1.5f, 0.7f, 1.0f, 0.0f);

    // High knockback, scaling with charge
    attacks.push_back(AttackBox(
        hitboxRect,
        15.0f * chargeMultiplier,   // Damage
        8.0f * chargeMultiplier,    // Base knockback
        0.3f,                       // Knockback scaling
        character.stateManager.isFacingRight ? 35.0f : 145.0f,  // Angle
        25,                         // Hitstun
        15                          // Shield stun
    ));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createUpSmash(float chargeTime) {
    std::vector<AttackBox> attacks;

    // Charge multiplier (1.0 to 1.5)
    float chargeMultiplier = 1.0f + std::min(chargeTime / 60.0f, 0.5f);

    // Tall vertical hitbox
    Rectangle hitboxRect = createHitboxRect(0.8f, 1.8f, 0.0f, -0.9f);

    // Strong vertical knockback
    attacks.push_back(AttackBox(
        hitboxRect,
        14.0f * chargeMultiplier,   // Damage
        7.0f * chargeMultiplier,    // Base knockback
        0.35f,                      // Knockback scaling
        90.0f,                      // Angle (straight up)
        20,                         // Hitstun
        15                          // Shield stun
    ));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createDownSmash(float chargeTime) {
    std::vector<AttackBox> attacks;

    // Charge multiplier (1.0 to 1.5)
    float chargeMultiplier = 1.0f + std::min(chargeTime / 60.0f, 0.5f);

    // Two hitboxes on both sides
    float hitboxWidth = character.width * 0.9f;
    float hitboxHeight = character.height * 0.5f;
    float hitboxY = character.physics.position.y + character.height/2 - hitboxHeight/2;

    // Left hitbox
    Rectangle leftHitboxRect = {
        character.physics.position.x - character.width/2 - hitboxWidth,
        hitboxY,
        hitboxWidth,
        hitboxHeight
    };

    attacks.push_back(AttackBox(
        leftHitboxRect,
        13.0f * chargeMultiplier,   // Damage
        6.0f * chargeMultiplier,    // Base knockback
        0.3f,                       // Knockback scaling
        20.0f,                      // Angle (semi-spike to the left)
        20,                         // Hitstun
        15                          // Shield stun
    ));

    // Right hitbox
    Rectangle rightHitboxRect = {
        character.physics.position.x + character.width/2,
        hitboxY,
        hitboxWidth,
        hitboxHeight
    };

    attacks.push_back(AttackBox(
        rightHitboxRect,
        13.0f * chargeMultiplier,   // Damage
        6.0f * chargeMultiplier,    // Base knockback
        0.3f,                       // Knockback scaling
        160.0f,                     // Angle (semi-spike to the right)
        20,                         // Hitstun
        15                          // Shield stun
    ));

    return attacks;
}

// Aerial attacks
std::vector<AttackBox> AttackFactory::createNeutralAir() {
    std::vector<AttackBox> attacks;

    // Circle hitbox around character - hits all around
    float hitboxRadius = character.width * 1.2f;
    Rectangle hitboxRect = {
        character.physics.position.x - hitboxRadius/2,
        character.physics.position.y - hitboxRadius/2,
        hitboxRadius,
        hitboxRadius
    };

    // Moderate damage, low knockback, faster than other aerials
    attacks.push_back(AttackBox(
        hitboxRect,
        8.0f,                       // Damage
        3.0f,                       // Base knockback
        0.12f,                      // Knockback scaling
        45.0f,                      // Angle
        15,                         // Hitstun
        12                          // Shield stun
    ));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createForwardAir() {
    std::vector<AttackBox> attacks;

    // Forward-reaching hitbox
    Rectangle hitboxRect = createHitboxRect(1.3f, 0.7f, 1.0f, 0.0f);

    // Good damage and knockback
    attacks.push_back(AttackBox(
        hitboxRect,
        10.0f,                      // Damage
        4.5f,                       // Base knockback
        0.2f,                       // Knockback scaling
        character.stateManager.isFacingRight ? 45.0f : 135.0f,  // Angle
        20,                         // Hitstun
        15                          // Shield stun
    ));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createBackAir() {
    std::vector<AttackBox> attacks;

    // Back hitbox (opposite of facing direction)
    Rectangle hitboxRect = createHitboxRect(1.1f, 0.8f, -1.0f, 0.0f);

    // Strong knockback - kill move
    attacks.push_back(AttackBox(
        hitboxRect,
        13.0f,                      // Damage
        6.0f,                       // Base knockback
        0.25f,                      // Knockback scaling
        character.stateManager.isFacingRight ? 135.0f : 45.0f,  // Angle
        25,                         // Hitstun
        15                          // Shield stun
    ));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createUpAir() {
    std::vector<AttackBox> attacks;

    // Upward hitbox
    Rectangle hitboxRect = createHitboxRect(0.8f, 1.1f, 0.0f, -1.0f);

    // Moderate damage, upward launch angle
    attacks.push_back(AttackBox(
        hitboxRect,
        9.0f,                       // Damage
        4.0f,                       // Base knockback
        0.2f,                       // Knockback scaling
        85.0f,                      // Angle (almost straight up)
        15,                         // Hitstun
        12                          // Shield stun
    ));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createDownAir() {
    std::vector<AttackBox> attacks;

    // Downward hitbox
    Rectangle hitboxRect = createHitboxRect(0.7f, 1.0f, 0.0f, 1.0f);

    // Strong spike with high risk/reward
    AttackBox spike(
        hitboxRect,
        14.0f,                      // Damage
        3.0f,                       // Base knockback
        0.15f,                      // Knockback scaling
        270.0f,                     // Angle (straight down)
        25,                         // Hitstun
        20                          // Shield stun
    );

    spike.canSpike = true;          // Can meteor smash opponents downward
    attacks.push_back(spike);

    return attacks;
}

// Special attacks
std::vector<AttackBox> AttackFactory::createNeutralSpecial() {
    std::vector<AttackBox> attacks;

    // Projectile hitbox
    Rectangle hitboxRect = createHitboxRect(0.8f, 0.6f, 1.0f, 0.0f);

    // Medium-speed projectile with moderate damage
    Vector2 projectileVel = {character.stateManager.isFacingRight ? 8.0f : -8.0f, 0};

    attacks.push_back(AttackBox(
        hitboxRect,
        8.0f,                       // Damage
        2.0f,                       // Base knockback
        0.1f,                       // Knockback scaling
        character.stateManager.isFacingRight ? 0.0f : 180.0f,  // Angle
        15,                         // Hitstun
        90,                         // Shield stun
        projectileVel,              // Velocity
        true                        // Is projectile
    ));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createSideSpecial() {
    std::vector<AttackBox> attacks;

    // Side special hitbox
    Rectangle hitboxRect = createHitboxRect(1.5f, 0.9f, 1.0f, 0.0f);

    attacks.push_back(AttackBox(
        hitboxRect,
        12.0f,                      // Damage
        6.0f,                       // Base knockback
        0.25f,                      // Knockback scaling
        character.stateManager.isFacingRight ? 30.0f : 150.0f,  // Angle
        25,                         // Hitstun
        25                          // Shield stun
    ));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createUpSpecial() {
    std::vector<AttackBox> attacks;

    // Up special hitbox
    Rectangle hitboxRect = createHitboxRect(1.2f, 1.5f, 0.0f, -0.8f);

    attacks.push_back(AttackBox(
        hitboxRect,
        10.0f,                      // Damage
        5.0f,                       // Base knockback
        0.2f,                       // Knockback scaling
        80.0f,                      // Angle (mostly upward)
        20,                         // Hitstun
        20                          // Shield stun
    ));

    return attacks;
}

std::vector<AttackBox> AttackFactory::createDownSpecial() {
    std::vector<AttackBox> attacks;

    // Counter/reflector hitbox
    Rectangle hitboxRect = {
        character.physics.position.x - character.width * 0.75f,
        character.physics.position.y - character.height * 0.75f,
        character.width * 1.5f,
        character.height * 1.5f
    };

    // Create a reflector hitbox with counter properties
    AttackBox reflector(
        hitboxRect,
        6.0f,                       // Damage
        3.0f,                       // Base knockback
        0.1f,                       // Knockback scaling
        45.0f,                      // Angle
        15,                         // Hitstun
        30,                         // Shield stun
        AttackBox::REFLECTOR        // Hitbox type
    );

    attacks.push_back(reflector);

    return attacks;
}

std::vector<AttackBox> AttackFactory::createGrab() {
    std::vector<AttackBox> attacks;

    // Grab hitbox
    Rectangle hitboxRect = createHitboxRect(0.8f, 0.7f, 1.0f, 0.0f);

    // Grab hitbox - no damage but initiates grab state
    AttackBox grabBox(
        hitboxRect,
        0.0f,                       // No direct damage
        0.0f,                       // No knockback
        0.0f,                       // No scaling
        0.0f,                       // No angle
        0,                          // No hitstun (handled by grab)
        10,                         // Shield stun if shielded
        AttackBox::GRAB             // Hitbox type
    );

    attacks.push_back(grabBox);

    return attacks;
}