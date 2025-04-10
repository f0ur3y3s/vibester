#include "../../include/character/Character.h"
#include "../../include/ParticleSystem.h"
#include "../../include/character/CharacterMovement.h"
#include "../../include/attacks/StandardAttacks.h"
#include "../../include/attacks/AerialAttacks.h"
#include "../../include/GameConfig.h"

using CharacterState::State;
using CharacterState::IDLE;
using CharacterState::RUNNING;
using CharacterState::JUMPING;
using CharacterState::FALLING;
using CharacterState::ATTACKING;
using CharacterState::SHIELDING;
using CharacterState::DODGING;
using CharacterState::HITSTUN;
using CharacterState::DYING;

using AttackType::Type;
using AttackType::NONE;
using AttackType::JAB;
using AttackType::FORWARD_TILT;
using AttackType::UP_TILT;
using AttackType::DOWN_TILT;
using AttackType::DASH_ATTACK;
using AttackType::FORWARD_SMASH;
using AttackType::UP_SMASH;
using AttackType::DOWN_SMASH;
using AttackType::NEUTRAL_AIR;
using AttackType::FORWARD_AIR;
using AttackType::BACK_AIR;
using AttackType::UP_AIR;
using AttackType::DOWN_AIR;
using AttackType::NEUTRAL_SPECIAL;
using AttackType::SIDE_SPECIAL;
using AttackType::UP_SPECIAL;
using AttackType::DOWN_SPECIAL;
using AttackType::GRAB;
using AttackType::PUMMEL;
using AttackType::FORWARD_THROW;
using AttackType::BACK_THROW;
using AttackType::UP_THROW;
using AttackType::DOWN_THROW;

// Main constructor
Character::Character(float x, float y, float w, float h, float spd, Color col, std::string n) {
    // Initialize physics system
    physics = CharacterPhysics(x, y);
    
    // Basic properties
    width = w;
    height = h;
    speed = spd;
    color = col;
    name = n;

    // Smash-style properties
    damagePercent = 0.0f;
    stocks = GameConfig::DEFAULT_STOCKS;

    // Animation variables
    currentFrame = 0;
    framesCounter = 0;
    framesSpeed = 8;

    // Grab state
    grabbedCharacter = nullptr;

    // Death animation
    deathRotation = 0;
    deathScale = 1.0f;
    deathVelocity = {0, 0};
    deathPosition = {0, 0};
}

// Basic geometry methods
Rectangle Character::getRect() {
    return {physics.position.x - width/2, physics.position.y - height/2, width, height};
}

Rectangle Character::getHurtbox() {
    // Hurtbox is slightly smaller than visual character size
    float hurtboxScale = 0.85f;
    float adjustedWidth = width * hurtboxScale;
    float adjustedHeight = height * hurtboxScale;
    return {physics.position.x - adjustedWidth/2, physics.position.y - adjustedHeight/2, 
            adjustedWidth, adjustedHeight};
}

void Character::changeState(CharacterState::State newState) {
    stateManager.changeState(newState);
}

// Reset attack state
void Character::resetAttackState() {
    stateManager.isAttacking = false;
    stateManager.currentAttack = NONE;
    stateManager.attackDuration = 0;
    stateManager.attackFrame = 0;
    attacks.clear();
    stateManager.canAttack = true;
}

bool Character::isOutOfBounds() {
    return physics.position.x < GameConfig::BLAST_ZONE_LEFT ||
           physics.position.x > GameConfig::BLAST_ZONE_RIGHT ||
           physics.position.y < GameConfig::BLAST_ZONE_TOP ||
           physics.position.y > GameConfig::BLAST_ZONE_BOTTOM;
}

// Accessor methods
float Character::getDamagePercent() const { 
    return damagePercent; 
}

int Character::getStocks() const { 
    return stocks; 
}

std::string Character::getName() const { 
    return name; 
}

// Main update method with physics and collision handling
void Character::update(std::vector<Platform>& platforms) {
    // Check for explosion threshold first
    checkForExplosion();

    // Clear attack boxes if dying or exploding
    if ((stateManager.isDying || stateManager.isExploding) && !attacks.empty()) {
        resetAttackState();
    }
    
    // Skip normal updates if exploding
    if (stateManager.isExploding) {
        updateExplosionAnimation();
        return;
    }

    // Skip updates if dying
    if (stateManager.isDying) {
        updateDeathAnimation();
        return;
    }

    // Update cooldowns and timers
    stateManager.updateCooldowns();
    stateManager.updateTimers();

    // Apply appropriate physics based on state
    bool onGround = false;

    // Variables for collision detection
    const int collisionSteps = 4; // Number of sub-steps for collision checking
    float stepX = physics.velocity.x / collisionSteps;
    float stepY = physics.velocity.y / collisionSteps;

    // Process current state
    switch (stateManager.state) {
        case IDLE:
        case RUNNING:
        case JUMPING:
        case FALLING:
            {
                // Apply gravity
                physics.applyGravity();

                // Handle collisions with sub-frame precision
                for (int step = 0; step < collisionSteps; step++) {
                    // Apply partial movement
                    physics.updatePositionPartial(stepX, stepY);

                    // Platform collision on each sub-step
                    for (auto& platform : platforms) {
                        Rectangle playerRect = getRect();
                        if (CheckCollisionRecs(playerRect, platform.rect)) {
                            // Handle collision based on platform type
                            if (platform.type == SOLID) {
                                // SOLID platforms have collision from all sides

                                // Top collision - only check if moving downward
                                if (stepY > 0) {
                                    if (playerRect.y + playerRect.height > platform.rect.y &&
                                        playerRect.y + playerRect.height < platform.rect.y + platform.rect.height / 2) {
                                        physics.position.y = platform.rect.y - height / 2;
                                        physics.velocity.y = 0;
                                        onGround = true;

                                        // Reset states that need ground
                                        if (stateManager.isJumping) stateManager.isJumping = false;
                                        stateManager.hasDoubleJump = true;
                                        stateManager.isHitstun = false;
                                        break; // Found a top collision, stop checking other platforms
                                    }
                                }

                                // Side collisions - prevent movement into platforms
                                // Only apply side collision if player's feet are below platform top
                                // AND player's top is not above platform bottom (to allow movement under platforms)
                                if (playerRect.y + playerRect.height > platform.rect.y + 5 &&
                                    playerRect.y < platform.rect.y + platform.rect.height) {
                                    // Right side collision - player moving right into platform left edge
                                    if (stepX > 0 &&
                                        playerRect.x + playerRect.width > platform.rect.x &&
                                        playerRect.x < platform.rect.x) {
                                        physics.position.x = platform.rect.x - width / 2;
                                        physics.velocity.x = 0;
                                    }
                                    // Left side collision - player moving left into platform right edge
                                    else if (stepX < 0 &&
                                            playerRect.x < platform.rect.x + platform.rect.width &&
                                            playerRect.x + playerRect.width > platform.rect.x + platform.rect.width) {
                                        physics.position.x = platform.rect.x + platform.rect.width + width / 2;
                                        physics.velocity.x = 0;
                                    }
                                }
                            }
                            else if (platform.type == PASSTHROUGH) {
                                // PASSTHROUGH platforms only have collision from above

                                // Top collision - only check if moving downward
                                if (stepY > 0) {
                                    // Previous position was above the platform
                                    if (playerRect.y + playerRect.height - stepY <= platform.rect.y) {
                                        physics.position.y = platform.rect.y - height / 2;
                                        physics.velocity.y = 0;
                                        onGround = true;

                                        // Reset states that need ground
                                        if (stateManager.isJumping) stateManager.isJumping = false;
                                        stateManager.hasDoubleJump = true;
                                        stateManager.isHitstun = false;
                                        break; // Found a top collision, stop checking other platforms
                                    }
                                }
                                // No side or bottom collisions for passthrough platforms
                            }
                        }
                    }
                }

                // Update state based on movement
                if (onGround) {
                    if (fabs(physics.velocity.x) > 0.5f) {
                        stateManager.changeState(RUNNING);
                    } else {
                        stateManager.changeState(IDLE);
                    }
                } else {
                    if (physics.velocity.y < 0) {
                        stateManager.changeState(JUMPING);
                    } else {
                        stateManager.changeState(FALLING);
                    }
                }

                // Apply friction
                physics.applyFriction(onGround);

                // Update attack positions if attacking
                if (stateManager.isAttacking) {
                    updateAttackPositions();
                    stateManager.attackFrame++;

                    // End attack when duration is over
                    if (stateManager.attackFrame >= stateManager.attackDuration) {
                        resetAttackState();
                    }
                }
            }
            break;

        case ATTACKING:
            {
                // Apply gravity during attacks
                physics.applyGravity();

                // Limited horizontal movement during attacks
                float modifiedVelocityX = physics.velocity.x * 0.5f;

                // Setup sub-frame precision for collision detection
                stepX = modifiedVelocityX / collisionSteps;
                stepY = physics.velocity.y / collisionSteps;

                for (int step = 0; step < collisionSteps; step++) {
                    // Apply partial movement
                    physics.updatePositionPartial(stepX, stepY);

                    // Platform collision handling - similar to above
                    for (auto& platform : platforms) {
                        Rectangle playerRect = getRect();
                        if (CheckCollisionRecs(playerRect, platform.rect)) {
                            if (platform.type == SOLID) {
                                // Full collision for solid platforms

                                // Top collision
                                if (stepY > 0) {
                                    if (playerRect.y + playerRect.height > platform.rect.y &&
                                        playerRect.y + playerRect.height < platform.rect.y + platform.rect.height / 2) {
                                        physics.position.y = platform.rect.y - height / 2;
                                        physics.velocity.y = 0;
                                        onGround = true;

                                        // Ground attacks continue
                                        // Air attacks may cancel on landing
                                        if (stateManager.currentAttack >= NEUTRAL_AIR && stateManager.currentAttack <= DOWN_AIR) {
                                            resetAttackState();
                                            stateManager.changeState(IDLE);
                                        }
                                        break; // Found a top collision, stop checking other platforms
                                    }
                                }

                                // Side collisions
                                if (playerRect.y + playerRect.height > platform.rect.y + 5 &&
                                    playerRect.y < platform.rect.y + platform.rect.height) {
                                    // Right side collision
                                    if (stepX > 0 &&
                                        playerRect.x + playerRect.width > platform.rect.x &&
                                        playerRect.x < platform.rect.x) {
                                        physics.position.x = platform.rect.x - width / 2;
                                        physics.velocity.x = 0;
                                    }
                                    // Left side collision
                                    else if (stepX < 0 &&
                                            playerRect.x < platform.rect.x + platform.rect.width &&
                                            playerRect.x + playerRect.width > platform.rect.x + platform.rect.width) {
                                        physics.position.x = platform.rect.x + platform.rect.width + width / 2;
                                        physics.velocity.x = 0;
                                    }
                                }
                            }
                            else if (platform.type == PASSTHROUGH) {
                                // Only top collision for passthrough platforms
                                if (stepY > 0) {
                                    // Check if coming from above
                                    if (playerRect.y + playerRect.height - stepY <= platform.rect.y) {
                                        physics.position.y = platform.rect.y - height / 2;
                                        physics.velocity.y = 0;
                                        onGround = true;

                                        // Check if air attack should cancel on landing
                                        if (stateManager.currentAttack >= NEUTRAL_AIR && stateManager.currentAttack <= DOWN_AIR) {
                                            resetAttackState();
                                            stateManager.changeState(IDLE);
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                // Update attack positions and increment attack frame
                updateAttackPositions();
                stateManager.attackFrame++;

                // End attack when duration is over and transition to appropriate state
                if (stateManager.attackFrame >= stateManager.attackDuration) {
                    resetAttackState();

                    // Return to appropriate state based on position and velocity
                    if (onGround) {
                        if (fabs(physics.velocity.x) > 0.5f) {
                            stateManager.changeState(RUNNING);
                        } else {
                            stateManager.changeState(IDLE);
                        }
                    } else if (physics.velocity.y < 0) {
                        stateManager.changeState(JUMPING);
                    } else {
                        stateManager.changeState(FALLING);
                    }
                }
            }
            break;

        case SHIELDING:
            // No movement while shielding
            physics.velocity.x = 0;
            physics.velocity.y = 0;
            break;

        case DODGING:
            {
                // Apply reduced gravity during dodges
                physics.velocity.y += GameConfig::GRAVITY * 0.5f;

                // Same collision logic as above for movement
                for (int step = 0; step < collisionSteps; step++) {
                    physics.updatePositionPartial(stepX, stepY);

                    // Platform collision
                    for (auto& platform : platforms) {
                        Rectangle playerRect = getRect();
                        if (CheckCollisionRecs(playerRect, platform.rect)) {
                            if (platform.type == SOLID) {
                                // Top collision
                                if (stepY > 0 && playerRect.y + playerRect.height > platform.rect.y &&
                                    playerRect.y + playerRect.height < platform.rect.y + platform.rect.height / 2) {
                                    physics.position.y = platform.rect.y - height / 2;
                                    physics.velocity.y = 0;
                                    onGround = true;
                                    break;
                                }

                                // Side collisions
                                if (playerRect.y + playerRect.height > platform.rect.y + 5 &&
                                    playerRect.y < platform.rect.y + platform.rect.height) {
                                    // Right collision
                                    if (stepX > 0 && playerRect.x + playerRect.width > platform.rect.x &&
                                        playerRect.x < platform.rect.x) {
                                        physics.position.x = platform.rect.x - width / 2;
                                        physics.velocity.x = 0;
                                    }
                                    // Left collision
                                    else if (stepX < 0 && playerRect.x < platform.rect.x + platform.rect.width &&
                                            playerRect.x + playerRect.width > platform.rect.x + platform.rect.width) {
                                        physics.position.x = platform.rect.x + platform.rect.width + width / 2;
                                        physics.velocity.x = 0;
                                    }
                                }
                            }
                            else if (platform.type == PASSTHROUGH && stepY > 0) {
                                // Passthrough top collision
                                if (playerRect.y + playerRect.height - stepY <= platform.rect.y) {
                                    physics.position.y = platform.rect.y - height / 2;
                                    physics.velocity.y = 0;
                                    onGround = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            break;

        case HITSTUN:
            {
                // Apply gravity
                physics.applyGravity();

                // Same collision logic as above
                for (int step = 0; step < collisionSteps; step++) {
                    physics.updatePositionPartial(stepX, stepY);

                    for (auto& platform : platforms) {
                        Rectangle playerRect = getRect();
                        if (CheckCollisionRecs(playerRect, platform.rect)) {
                            if (platform.type == SOLID) {
                                // Top collision
                                if (stepY > 0 && playerRect.y + playerRect.height > platform.rect.y &&
                                    playerRect.y + playerRect.height < platform.rect.y + platform.rect.height / 2) {
                                    physics.position.y = platform.rect.y - height / 2;
                                    physics.velocity.y = 0;
                                    onGround = true;
                                    break;
                                }

                                // Side collisions
                                if (playerRect.y + playerRect.height > platform.rect.y + 5 &&
                                    playerRect.y < platform.rect.y + platform.rect.height) {
                                    // Right collision
                                    if (stepX > 0 && playerRect.x + playerRect.width > platform.rect.x &&
                                        playerRect.x < platform.rect.x) {
                                        physics.position.x = platform.rect.x - width / 2;
                                        physics.velocity.x = 0;
                                    }
                                    // Left collision
                                    else if (stepX < 0 && playerRect.x < platform.rect.x + platform.rect.width &&
                                            playerRect.x + playerRect.width > platform.rect.x + platform.rect.width) {
                                        physics.position.x = platform.rect.x + platform.rect.width + width / 2;
                                        physics.velocity.x = 0;
                                    }
                                }
                            }
                            else if (platform.type == PASSTHROUGH && stepY > 0) {
                                // Passthrough top collision
                                if (playerRect.y + playerRect.height - stepY <= platform.rect.y) {
                                    physics.position.y = platform.rect.y - height / 2;
                                    physics.velocity.y = 0;
                                    onGround = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            break;
    }

    // Check if out of bounds
    if (isOutOfBounds()) {
        startDeathAnimation();
    }

    // Update hit effects
    for (int i = 0; i < hitEffects.size(); i++) {
        if (!hitEffects[i].update()) {
            hitEffects.erase(hitEffects.begin() + i);
            i--;
        }
    }
}

void Character::draw() {
    // Skip normal drawing if exploding
    if (stateManager.isExploding) {
        drawExplosionAnimation();
        return;
    }

    // Skip normal drawing if dying
    if (stateManager.isDying) {
        drawDeathAnimation();
        return;
    }

    // Visual effects for states
    Color drawColor = color;

    // Flashing for invincibility
    if (stateManager.isInvincible) {
        if ((framesCounter / 3) % 2 == 0) {
            drawColor.a = 128; // Half opacity
        }
    }

    // Damage gradient
    if (damagePercent > 0) {
        // Gradually shift to red as damage increases
        float damageRatio = std::min(damagePercent / 150.0f, 1.0f);
        drawColor.r = std::min(255, drawColor.r + static_cast<int>(damageRatio * 100));
        drawColor.g = std::max(0, drawColor.g - static_cast<int>(damageRatio * 80));
        drawColor.b = std::max(0, drawColor.b - static_cast<int>(damageRatio * 80));
    }

    // Basic character drawing
    DrawRectangle(
        static_cast<int>(physics.position.x - width/2),
        static_cast<int>(physics.position.y - height/2),
        static_cast<int>(width),
        static_cast<int>(height),
        drawColor
    );

    // Direction indicator (eyes/face)
    float eyeOffset = stateManager.isFacingRight ? width * 0.2f : -width * 0.2f;
    DrawCircle(
        static_cast<int>(physics.position.x + eyeOffset),
        static_cast<int>(physics.position.y - height * 0.1f),
        width * 0.15f,
        BLACK
    );

    // Shield visualization
    if (stateManager.isShielding) {
        float shieldRatio = stateManager.shieldHealth / GameConfig::MAX_SHIELD_HEALTH;
        float shieldSize = (width + height) * 0.4f * shieldRatio;
        Color shieldColor = {100, 200, 255, 128}; // Semi-transparent blue

        // Shield color shifts to red as it weakens
        shieldColor.g = static_cast<unsigned char>(200 * shieldRatio);
        shieldColor.b = static_cast<unsigned char>(255 * shieldRatio);

        DrawCircleV(physics.position, shieldSize, shieldColor);
    }

    // Draw hitboxes if attacking and not dying/exploding (for debug)
    if (stateManager.isAttacking && !stateManager.isDying && !stateManager.isExploding) {
        for (auto& attack : attacks) {
            attack.draw(true);
        }
    }

    // Draw hit effects
    for (auto& effect : hitEffects) {
        effect.draw();
    }

    // Draw percentage above character
    char damageText[10];
    sprintf(damageText, "%.0f%%", damagePercent);
    DrawText(
        damageText,
        static_cast<int>(physics.position.x - width/2),
        static_cast<int>(physics.position.y - height - 20),
        20,
        WHITE
    );

    // Animation counter
    framesCounter++;
}

// Movement method delegations
void Character::jump() {
    CharacterMovement::executeJump(this);
}

void Character::doubleJump() {
    CharacterMovement::executeDoubleJump(this);
}

void Character::moveLeft() {
    CharacterMovement::executeMoveLeft(this);
}

void Character::moveRight() {
    CharacterMovement::executeMoveRight(this);
}

void Character::fastFall() {
    CharacterMovement::executeFastFall(this);
}

void Character::dropThroughPlatform() {
    CharacterMovement::executeDropThroughPlatform(this);
}

// Defense method delegations
void Character::shield() {
    CharacterMovement::executeShield(this);
}

void Character::releaseShield() {
    CharacterMovement::executeReleaseShield(this);
}

void Character::spotDodge() {
    CharacterMovement::executeSpotDodge(this);
}

void Character::forwardDodge() {
    CharacterMovement::executeForwardDodge(this);
}

void Character::backDodge() {
    CharacterMovement::executeBackDodge(this);
}

void Character::airDodge(float dirX, float dirY) {
    CharacterMovement::executeAirDodge(this, dirX, dirY);
}

// Standard attack delegations
void Character::jab() {
    StandardAttacks::executeJab(this);
}

void Character::forwardTilt() {
    StandardAttacks::executeForwardTilt(this);
}

void Character::upTilt() {
    StandardAttacks::executeUpTilt(this);
}

void Character::downTilt() {
    StandardAttacks::executeDownTilt(this);
}

void Character::dashAttack() {
    StandardAttacks::executeDashAttack(this);
}

// Smash attack delegations
void Character::forwardSmash(float chargeTime) {
    StandardAttacks::executeForwardSmash(this, chargeTime);
}

void Character::upSmash(float chargeTime) {
    StandardAttacks::executeUpSmash(this, chargeTime);
}

void Character::downSmash(float chargeTime) {
    StandardAttacks::executeDownSmash(this, chargeTime);
}

// Aerial attack delegations
void Character::neutralAir() {
    AerialAttacks::executeNeutralAir(this);
}

void Character::forwardAir() {
    AerialAttacks::executeForwardAir(this);
}

void Character::backAir() {
    AerialAttacks::executeBackAir(this);
}

void Character::upAir() {
    AerialAttacks::executeUpAir(this);
}

void Character::downAir() {
    AerialAttacks::executeDownAir(this);
}

// Special attacks implementation
void Character::neutralSpecial() {
    if (stateManager.canAttack && !stateManager.specialNeutralCD.isActive()) {
        resetAttackState();
        stateManager.isAttacking = true;
        stateManager.currentAttack = NEUTRAL_SPECIAL;
        stateManager.attackDuration = 30;
        stateManager.changeState(ATTACKING);
        stateManager.specialNeutralCD.reset();

        // Create projectile hitbox
        float hitboxWidth = width * 0.5f;
        float hitboxHeight = width * 0.5f;
        float hitboxX = stateManager.isFacingRight
                            ? physics.position.x + width
                            : physics.position.x - width - hitboxWidth;
        float hitboxY = physics.position.y - hitboxHeight / 2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(hitboxRect, 8.0f, 3.0f, 0.1f, stateManager.isFacingRight ? 0.0f : 180.0f, 10, 10));
    }
}

void Character::sideSpecial() {
    if (stateManager.canAttack && !stateManager.specialSideCD.isActive()) {
        resetAttackState();
        stateManager.isAttacking = true;
        stateManager.currentAttack = SIDE_SPECIAL;
        stateManager.attackDuration = 35;
        stateManager.changeState(ATTACKING);
        stateManager.specialSideCD.reset();

        // Add momentum to side special
        physics.velocity.x = stateManager.isFacingRight ? speed * 2.0f : -speed * 2.0f;

        // Create side special hitbox
        float hitboxWidth = width * 1.2f;
        float hitboxHeight = height * 0.7f;
        float hitboxX = stateManager.isFacingRight
                            ? physics.position.x + width / 2
                            : physics.position.x - width / 2 - hitboxWidth;
        float hitboxY = physics.position.y - hitboxHeight / 2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(hitboxRect, 12.0f, 6.0f, 0.2f, stateManager.isFacingRight ? 45.0f : 135.0f, 15, 15));
    }
}

void Character::upSpecial() {
    if (stateManager.canAttack && !stateManager.specialUpCD.isActive()) {
        resetAttackState();
        stateManager.isAttacking = true;
        stateManager.currentAttack = UP_SPECIAL;
        stateManager.attackDuration = 40;
        stateManager.changeState(ATTACKING);
        stateManager.specialUpCD.reset();

        // Recovery move - vertical boost
        physics.velocity.y = GameConfig::JUMP_FORCE * 1.5f;
        physics.velocity.x = stateManager.isFacingRight ? speed * 0.5f : -speed * 0.5f;

        // Restore double jump for recovery
        stateManager.hasDoubleJump = true;

        // Create up special hitbox that follows the character
        float hitboxWidth = width * 1.1f;
        float hitboxHeight = height * 1.1f;
        float hitboxX = physics.position.x - hitboxWidth / 2;
        float hitboxY = physics.position.y - hitboxHeight / 2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(hitboxRect, 7.0f, 5.0f, 0.15f, 80.0f, 12, 12));
    }
}

void Character::downSpecial() {
    if (stateManager.canAttack && !stateManager.specialDownCD.isActive()) {
        resetAttackState();
        stateManager.isAttacking = true;
        stateManager.currentAttack = DOWN_SPECIAL;
        stateManager.attackDuration = 45;
        stateManager.changeState(ATTACKING);
        stateManager.specialDownCD.reset();

        // Counter move - delay the actual hitbox creation for timing
        // Actual hitbox will be created during the attack update
        // For now just set up the state

        // Create visual indicator hitbox (no damage)
        float hitboxWidth = width * 1.5f;
        float hitboxHeight = height * 1.5f;
        float hitboxX = physics.position.x - hitboxWidth / 2;
        float hitboxY = physics.position.y - hitboxHeight / 2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        // Counter hitboxes should be created dynamically when hit
    }
}

// Grab and throw implementation
void Character::grab() {
    if (stateManager.canAttack && stateManager.state != JUMPING && stateManager.state != FALLING) {
        resetAttackState();
        stateManager.isAttacking = true;
        stateManager.currentAttack = GRAB;
        stateManager.attackDuration = 20;
        stateManager.changeState(ATTACKING);

        // Create grab hitbox
        float hitboxWidth = width * 0.6f;
        float hitboxHeight = height * 0.6f;
        float hitboxX = stateManager.isFacingRight
                            ? physics.position.x + width / 2
                            : physics.position.x - width / 2 - hitboxWidth;
        float hitboxY = physics.position.y - hitboxHeight / 2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        AttackBox grabBox(hitboxRect, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0);
        grabBox.type = AttackBox::GRAB;
        attacks.push_back(grabBox);
    }
}

void Character::pummel() {
    if (stateManager.isGrabbing && grabbedCharacter != nullptr) {
        // Apply damage to grabbed opponent
        grabbedCharacter->applyDamage(2.0f);
        createHitEffect(grabbedCharacter->physics.position);
        
        // Extend grab duration slightly
        stateManager.grabFrame -= 10;
        if (stateManager.grabFrame < 0) stateManager.grabFrame = 0;
    }
}

void Character::forwardThrow() {
    if (stateManager.isGrabbing && grabbedCharacter != nullptr) {
        // Release and throw forward
        float direction = stateManager.isFacingRight ? 1.0f : -1.0f;
        
        // Apply damage and knockback
        grabbedCharacter->applyDamage(8.0f);
        grabbedCharacter->applyKnockback(8.0f, 5.0f, 0.15f, direction, -0.2f);
        
        // Create hit effect
        createHitEffect(grabbedCharacter->physics.position);
        
        // Release the grab
        releaseGrab();
    }
}

void Character::backThrow() {
    if (stateManager.isGrabbing && grabbedCharacter != nullptr) {
        // Release and throw backward
        float direction = stateManager.isFacingRight ? -1.0f : 1.0f;
        
        // Apply damage and knockback
        grabbedCharacter->applyDamage(10.0f);
        grabbedCharacter->applyKnockback(10.0f, 6.0f, 0.2f, direction, -0.1f);
        
        // Create hit effect
        createHitEffect(grabbedCharacter->physics.position);
        
        // Release the grab
        releaseGrab();
    }
}

void Character::upThrow() {
    if (stateManager.isGrabbing && grabbedCharacter != nullptr) {
        // Release and throw upward
        
        // Apply damage and knockback
        grabbedCharacter->applyDamage(7.0f);
        grabbedCharacter->applyKnockback(7.0f, 5.0f, 0.15f, 0.0f, -1.0f);
        
        // Create hit effect
        createHitEffect(grabbedCharacter->physics.position);
        
        // Release the grab
        releaseGrab();
    }
}

void Character::downThrow() {
    if (stateManager.isGrabbing && grabbedCharacter != nullptr) {
        // Release and throw downward (bounce)
        
        // Apply damage and knockback
        grabbedCharacter->applyDamage(6.0f);
        grabbedCharacter->applyKnockback(6.0f, 4.0f, 0.1f, 0.0f, 0.5f);
        
        // Create hit effect
        createHitEffect(grabbedCharacter->physics.position);
        
        // Release the grab
        releaseGrab();
    }
}

void Character::releaseGrab() {
    if (stateManager.isGrabbing && grabbedCharacter != nullptr) {
        stateManager.isGrabbing = false;
        grabbedCharacter = nullptr;
        stateManager.grabFrame = 0;
    }
}

// Combat implementation
bool Character::checkHit(Character& other) {
    // Skip if the other character is invincible, dying, or exploding
    if (other.stateManager.isInvincible || other.stateManager.isDying || other.stateManager.isExploding) return false;

    // Check each attack hitbox
    for (auto& attack : attacks) {
        Rectangle otherHurtbox = other.getHurtbox();

        if (CheckCollisionRecs(attack.rect, otherHurtbox)) {
            // Handle different hitbox types
            switch (attack.type) {
                case AttackBox::GRAB:
                    // Initiate grab
                    if (!other.stateManager.isShielding) {
                        stateManager.isGrabbing = true;
                        grabbedCharacter = &other;
                        stateManager.grabDuration = 120; // Hold for 2 seconds max
                        stateManager.grabFrame = 0;

                        // Position the grabbed character
                        float grabOffset = stateManager.isFacingRight ? width : -width;
                        other.physics.position.x = physics.position.x + grabOffset;
                        other.physics.position.y = physics.position.y;

                        other.physics.velocity = {0, 0};
                        other.stateManager.isHitstun = true;
                        other.stateManager.hitstunFrames = 1; // Keep in hitstun while grabbed
                    }
                    break;

                case AttackBox::NORMAL:
                default:
                    // Handle shield
                    if (other.stateManager.isShielding) {
                        // Reduce shield health
                        other.stateManager.shieldHealth -= attack.damage * GameConfig::SHIELD_DAMAGE_MULTIPLIER;

                        // Shield break
                        if (other.stateManager.shieldHealth <= 0) {
                            other.stateManager.shieldHealth = 0;
                            other.stateManager.isShielding = false;
                            other.stateManager.isHitstun = true;
                            other.stateManager.hitstunFrames = GameConfig::SHIELD_BREAK_STUN;

                            // Apply upward knockback
                            other.physics.velocity.y = -8.0f;
                        }

                        // Shield stun
                        other.stateManager.isHitstun = true;
                        other.stateManager.hitstunFrames = GameConfig::SHIELD_STUN_FRAMES + attack.shieldStun;
                    } else {
                        // Apply damage and knockback
                        other.applyDamage(attack.damage);

                        // Calculate knockback direction
                        float knockbackAngle = attack.knockbackAngle * DEG2RAD;
                        float directionX = cosf(knockbackAngle);
                        float directionY = sinf(knockbackAngle);

                        // Apply knockback
                        other.applyKnockback(
                            attack.damage,
                            attack.baseKnockback,
                            attack.knockbackScaling,
                            directionX,
                            directionY
                        );

                        // Create hit effect
                        Vector2 hitPos = {
                            (attack.rect.x + attack.rect.width/2 + otherHurtbox.x + otherHurtbox.width/2) / 2,
                            (attack.rect.y + attack.rect.height/2 + otherHurtbox.y + otherHurtbox.height/2) / 2
                        };
                        createHitEffect(hitPos);
                    }
                    break;
            }

            return true;
        }
    }

    return false;
}

void Character::applyDamage(float damage) {
    damagePercent += damage;
    if (damagePercent > GameConfig::MAX_DAMAGE) {
        damagePercent = GameConfig::MAX_DAMAGE;
    }
}

void Character::applyKnockback(float damage, float baseKnockback, float knockbackScaling, 
                               float directionX, float directionY) {
    // Calculate knockback based on damage and scaling
    float damageMultiplier = 1.0f + (damagePercent * GameConfig::DAMAGE_SCALING);
    float knockbackMagnitude = baseKnockback + (knockbackScaling * damageMultiplier);

    // Apply knockback velocity
    physics.velocity.x = directionX * knockbackMagnitude;
    physics.velocity.y = directionY * knockbackMagnitude;

    // Cap vertical velocity to prevent extreme values
    physics.capVerticalVelocity(30.0f);

    // Set hitstun frames based on knockback
    stateManager.isHitstun = true;
    stateManager.hitstunFrames = static_cast<int>(knockbackMagnitude * GameConfig::HITSTUN_MULTIPLIER);

    // Change state to hitstun
    stateManager.changeState(HITSTUN);

    // Reset aerial state
    stateManager.isJumping = false;
    stateManager.hasDoubleJump = false;  // Lose double jump when hit hard
}

void Character::createHitEffect(Vector2 position) {
    hitEffects.push_back(HitEffect(position, color));
}

// Death animation implementation
void Character::startDeathAnimation() {
    if (!stateManager.isDying) {
        stateManager.isDying = true;
        stateManager.state = DYING;
        stateManager.deathFrame = 0;
        deathRotation = 0;
        deathScale = 1.0f;

        // Clear all attacks when dying
        resetAttackState();

        // Set initial death velocity based on current velocity
        deathVelocity = physics.velocity;
        if (deathVelocity.y > -5.0f) deathVelocity.y = -5.0f; // Ensure upward motion

        // Store position at death
        deathPosition = physics.position;

        // Reduce stock
        stocks--;
    }
}

void Character::updateDeathAnimation() {
    stateManager.deathFrame++;

    // Update death position with velocity
    deathPosition.x += deathVelocity.x;
    deathPosition.y += deathVelocity.y;

    // Apply gravity to death animation
    deathVelocity.y += GameConfig::GRAVITY * 0.5f;

    // Spin and shrink
    deathRotation += 15.0f;
    deathScale = std::max(0.0f, 1.0f - static_cast<float>(stateManager.deathFrame) / stateManager.deathDuration);

    // End death animation
    if (stateManager.deathFrame >= stateManager.deathDuration) {
        stateManager.isDying = false;

        if (stocks > 0) {
            // Reset for respawn
            damagePercent = 0;
            physics.velocity = {0, 0};
            stateManager.isInvincible = true;
            stateManager.invincibilityFrames = 120; // 2 seconds of invincibility

            // Respawn at center top
            physics.position.x = GameConfig::SCREEN_WIDTH / 2;
            physics.position.y = 100;

            stateManager.changeState(FALLING);
        }
    }
}

void Character::drawDeathAnimation() {
    // Draw spinning, shrinking character
    Rectangle destRect = {
        deathPosition.x,
        deathPosition.y,
        width * deathScale,
        height * deathScale
    };

    // Center the rectangle for rotation
    destRect.x -= destRect.width / 2;
    destRect.y -= destRect.height / 2;

    // Draw rotated rectangle
    DrawRectanglePro(
        destRect,
        {destRect.width / 2, destRect.height / 2},
        deathRotation,
        color
    );

    // Star burst effect near end of animation
    if (stateManager.deathFrame > stateManager.deathDuration * 0.7f && stateManager.deathFrame % 3 == 0) {
        float starAngle = static_cast<float>(GetRandomValue(0, 360));
        float starDist = static_cast<float>(GetRandomValue(10, 30));
        Vector2 starPos = {
            deathPosition.x + cosf(starAngle * DEG2RAD) * starDist,
            deathPosition.y + sinf(starAngle * DEG2RAD) * starDist
        };

        DrawCircleV(starPos, 5.0f * deathScale, WHITE);
    }
}

void Character::respawn(Vector2 spawnPoint) {
    physics.position = spawnPoint;
    physics.velocity = {0, 0};
    damagePercent = 0;
    stateManager.isInvincible = true;
    stateManager.invincibilityFrames = 120; // 2 seconds of invincibility
    resetAttackState();
    stateManager.changeState(FALLING);
}

// Attack position updates
void Character::updateAttackPositions() {
    for (auto& attack : attacks) {
        // Position the attack box relative to the character
        float offsetX = stateManager.isFacingRight ? 1.0f : -1.0f;
        float boxCenterX = physics.position.x + (attack.rect.width / 2) * offsetX;

        // Adjust based on attack box original position
        attack.rect.x = boxCenterX - (attack.rect.width / 2);
        attack.rect.y = physics.position.y - (attack.rect.height / 2);
    }
}

// Explosion mechanics implementation
void Character::checkForExplosion() {
    // Check if damage threshold reached for explosion
    if (damagePercent >= EXPLOSION_DAMAGE_THRESHOLD && !stateManager.isDying && !stateManager.isExploding) {
        startExplosionAnimation();
    }
}

void Character::startExplosionAnimation() {
    stateManager.isExploding = true;
    stateManager.explosionFrame = 0;
    stateManager.explosionDuration = 60; // 1 second explosion
    explosionParticles.clear();
    
    // Clear all attacks when exploding
    resetAttackState();

    // Create explosion particles
    for (int i = 0; i < 150; i++) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(5, 15);
        Vector2 particleVel = {
            cosf(angle) * speed,
            sinf(angle) * speed
        };

        float size = GetRandomValue(3, 12);
        int lifespan = GetRandomValue(30, 90);

        // Create varied colored particles
        Color particleColor;
        int colorChoice = GetRandomValue(0, 4);
        switch (colorChoice) {
            case 0: particleColor = RED; break;
            case 1: particleColor = ORANGE; break;
            case 2: particleColor = YELLOW; break;
            case 3: particleColor = color; break;
            case 4: particleColor = WHITE; break;
        }

        explosionParticles.push_back(Particle(physics.position, particleVel, size, lifespan, particleColor));
    }

    // Reduce stock after explosion
    stocks--;

    // Reset damage after explosion
    damagePercent = 0;

    // Apply a dramatic screen shake effect and flash
    // (These visual effects are handled in Game.cpp)
}

void Character::updateExplosionAnimation() {
    stateManager.explosionFrame++;

    // Update existing explosion particles
    for (int i = 0; i < explosionParticles.size(); i++) {
        if (!explosionParticles[i].update()) {
            explosionParticles.erase(explosionParticles.begin() + i);
            i--;
        }
    }

    // Add new particles during the initial phase of explosion
    if (stateManager.explosionFrame < stateManager.explosionDuration / 2) {
        int particlesToAdd = 5;
        for (int i = 0; i < particlesToAdd; i++) {
            float angle = GetRandomValue(0, 360) * DEG2RAD;
            float speed = GetRandomValue(3, 10);
            Vector2 particleVel = {
                cosf(angle) * speed,
                sinf(angle) * speed
            };

            float size = GetRandomValue(2, 8);
            int lifespan = GetRandomValue(20, 60);

            // Create more colorful particles for secondary explosion
            Color particleColor;
            int colorChoice = GetRandomValue(0, 3);
            switch (colorChoice) {
                case 0: particleColor = RED;
                    break;
                case 1: particleColor = ORANGE;
                    break;
                case 2: particleColor = YELLOW;
                    break;
                case 3: particleColor = WHITE;
                    break;
            }

            explosionParticles.push_back(Particle(physics.position, particleVel, size, lifespan, particleColor));
        }
    }

    // End explosion animation
    if (stateManager.explosionFrame >= stateManager.explosionDuration) {
        stateManager.isExploding = false;

        // Respawn after explosion
        if (stocks > 0) {
            // Reset for respawn
            damagePercent = 0;
            physics.velocity = {0, 0};
            stateManager.isInvincible = true;
            stateManager.invincibilityFrames = 120; // 2 seconds of invincibility

            // Respawn at center top
            physics.position.x = GameConfig::SCREEN_WIDTH / 2;
            physics.position.y = 100;

            stateManager.changeState(FALLING);
        }
    }
}

void Character::drawExplosionAnimation() {
    // Draw explosion particles
    for (auto& particle : explosionParticles) {
        particle.draw();
    }

    // Draw shockwave effect
    float shockwaveRadius = stateManager.explosionFrame * 8.0f;
    float alpha = 255 * (1.0f - (float)stateManager.explosionFrame / stateManager.explosionDuration);
    Color shockwaveColor = {255, 200, 50, (unsigned char)alpha};

    DrawCircleLines(physics.position.x, physics.position.y, shockwaveRadius, shockwaveColor);
    DrawCircleLines(physics.position.x, physics.position.y, shockwaveRadius * 0.7f, shockwaveColor);

    // Draw flash effect in early frames
    if (stateManager.explosionFrame < 10) {
        Color flashColor = {255, 255, 255, (unsigned char)(255 * (1.0f - stateManager.explosionFrame / 10.0f))};
        DrawRectangle(0, 0, GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT, flashColor);
    }
}