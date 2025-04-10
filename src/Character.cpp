#include "Character.h"
#include "ParticleSystem.h"
#include <cmath>
#include <algorithm>

// HitEffect implementation
HitEffect::HitEffect(Vector2 pos, Color col) {
    position = pos;
    color = col;
    duration = 15;
    currentFrame = 0;
    size = 30.0f;
}

bool HitEffect::update() {
    currentFrame++;
    size -= 1.5f;
    return currentFrame < duration;
}

void HitEffect::draw() {
    float alpha = 1.0f - (float)currentFrame / duration;
    Color effectColor = {color.r, color.g, color.b, (unsigned char)(255 * alpha)};
    DrawCircleV(position, size, effectColor);

    // Draw impact lines
    for (int i = 0; i < 8; i++) {
        float angle = i * 45.0f * DEG2RAD;
        float lineLength = size * 1.5f * (1.0f - (float)currentFrame / duration);
        Vector2 end = {
            position.x + cos(angle) * lineLength,
            position.y + sin(angle) * lineLength
        };
        DrawLineEx(position, end, 3.0f, effectColor);
    }
}

// Character implementation
Character::Character(float x, float y, float w, float h, float spd, Color col, std::string n) {
    // Basic properties
    position = {x, y};
    velocity = {0, 0};
    width = w;
    height = h;
    speed = spd;
    color = col;
    name = n;

    // Smash-style properties
    damagePercent = 0.0f;
    stocks = DEFAULT_STOCKS;
    isInvincible = false;
    invincibilityFrames = 0;

    // State flags
    isFacingRight = true;
    isJumping = false;
    hasDoubleJump = true;
    isAttacking = false;
    isShielding = false;
    shieldHealth = MAX_SHIELD_HEALTH;
    isDodging = false;
    dodgeFrames = 0;
    isFastFalling = false;

    // Animation variables
    currentFrame = 0;
    framesCounter = 0;
    framesSpeed = 8;
    isHitstun = false;
    hitstunFrames = 0;

    // State machine
    state = IDLE;

    // Attack state
    currentAttack = NONE;
    attackDuration = 0;
    attackFrame = 0;
    canAttack = true;

    // Grab state
    isGrabbing = false;
    grabbedCharacter = nullptr;
    grabDuration = 0;
    grabFrame = 0;

    // Cooldowns
    specialNeutralCD = {120, 0};
    specialSideCD = {90, 0};
    specialUpCD = {60, 0};
    specialDownCD = {120, 0};
    dodgeCD = {DODGE_COOLDOWN, 0};

    // Death animation
    isDying = false;
    deathRotation = 0;
    deathScale = 1.0f;
    deathDuration = 60;
    deathFrame = 0;
    deathVelocity = {0, 0};
    deathPosition = {0, 0};
}

Rectangle Character::getRect() {
    return {position.x - width/2, position.y - height/2, width, height};
}

Rectangle Character::getHurtbox() {
    // Hurtbox is slightly smaller than visual character size
    float hurtboxScale = 0.85f;
    float adjustedWidth = width * hurtboxScale;
    float adjustedHeight = height * hurtboxScale;
    return {position.x - adjustedWidth/2, position.y - adjustedHeight/2, adjustedWidth, adjustedHeight};
}

void Character::update(std::vector<Platform>& platforms) {
    // Skip updates if dying
    if (isDying) {
        updateDeathAnimation();
        return;
    }

    // Update cooldowns
    updateCooldowns();

    // Variables used across all states
    bool onGround = false;

    // Variables for collision detection
    const int collisionSteps = 4; // Number of sub-steps for collision checking
    float stepX = 0;
    float stepY = 0;
    float modifiedVelocityX = 0;

    // Process current state
    switch (state) {
        case IDLE:
        case RUNNING:
        case JUMPING:
        case FALLING:
            // Apply gravity
            if (isFastFalling) {
                velocity.y += FAST_FALL_GRAVITY;
            } else {
                velocity.y += GRAVITY;
            }

            // Setup sub-frame precision for collision detection
            stepX = velocity.x / collisionSteps;
            stepY = velocity.y / collisionSteps;

            for (int step = 0; step < collisionSteps; step++) {
                // Apply partial movement
                position.x += stepX;
                position.y += stepY;

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
                                    position.y = platform.rect.y - height / 2;
                                    velocity.y = 0;
                                    onGround = true;

                                    // Reset states that need ground
                                    if (isJumping) isJumping = false;
                                    hasDoubleJump = true;
                                    isHitstun = false;
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
                                    position.x = platform.rect.x - width / 2;
                                    velocity.x = 0;
                                }
                                // Left side collision - player moving left into platform right edge
                                else if (stepX < 0 &&
                                         playerRect.x < platform.rect.x + platform.rect.width &&
                                         playerRect.x + playerRect.width > platform.rect.x + platform.rect.width) {
                                    position.x = platform.rect.x + platform.rect.width + width / 2;
                                    velocity.x = 0;
                                }
                            }
                        }
                        else if (platform.type == PASSTHROUGH) {
                            // PASSTHROUGH platforms only have collision from above

                            // Top collision - only check if moving downward
                            if (stepY > 0) {
                                // Previous position was above the platform
                                if (playerRect.y + playerRect.height - stepY <= platform.rect.y) {
                                    position.y = platform.rect.y - height / 2;
                                    velocity.y = 0;
                                    onGround = true;

                                    // Reset states that need ground
                                    if (isJumping) isJumping = false;
                                    hasDoubleJump = true;
                                    isHitstun = false;
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
                if (fabs(velocity.x) > 0.5f) {
                    changeState(RUNNING);
                } else {
                    changeState(IDLE);
                }
            } else {
                if (velocity.y < 0) {
                    changeState(JUMPING);
                } else {
                    changeState(FALLING);
                }
            }

            // Apply friction
            if (onGround) {
                velocity.x *= 0.9f;
                if (fabs(velocity.x) < 0.1f) velocity.x = 0;
            } else {
                velocity.x *= 0.98f; // Less friction in air
            }

            // Update attack positions if attacking
            if (isAttacking) {
                updateAttackPositions();
                attackFrame++;

                // End attack when duration is over
                if (attackFrame >= attackDuration) {
                    resetAttackState();
                }
            }

            // Update hitstun
            if (isHitstun) {
                hitstunFrames--;
                if (hitstunFrames <= 0) {
                    isHitstun = false;
                }
            }

            // Update invincibility
            if (isInvincible) {
                invincibilityFrames--;
                if (invincibilityFrames <= 0) {
                    isInvincible = false;
                }
            }
            break;

        // Similar changes for other states...
        case ATTACKING:
            // Apply gravity during attacks unless it's a specific air attack type
            velocity.y += GRAVITY;

            // Limited horizontal movement during attacks
            modifiedVelocityX = velocity.x * 0.5f;

            // Setup sub-frame precision for collision detection
            stepX = modifiedVelocityX / collisionSteps;
            stepY = velocity.y / collisionSteps;

            for (int step = 0; step < collisionSteps; step++) {
                // Apply partial movement
                position.x += stepX;
                position.y += stepY;

                // Platform collision on each sub-step
                for (auto& platform : platforms) {
                    Rectangle playerRect = getRect();
                    if (CheckCollisionRecs(playerRect, platform.rect)) {
                        if (platform.type == SOLID) {
                            // Full collision for solid platforms

                            // Top collision
                            if (stepY > 0) {
                                if (playerRect.y + playerRect.height > platform.rect.y &&
                                    playerRect.y + playerRect.height < platform.rect.y + platform.rect.height / 2) {
                                    position.y = platform.rect.y - height / 2;
                                    velocity.y = 0;
                                    onGround = true;

                                    // Ground attacks continue
                                    // Air attacks may cancel on landing
                                    if (currentAttack >= NEUTRAL_AIR && currentAttack <= DOWN_AIR) {
                                        resetAttackState();
                                        changeState(IDLE);
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
                                    position.x = platform.rect.x - width / 2;
                                    velocity.x = 0;
                                }
                                // Left side collision
                                else if (stepX < 0 &&
                                         playerRect.x < platform.rect.x + platform.rect.width &&
                                         playerRect.x + playerRect.width > platform.rect.x + platform.rect.width) {
                                    position.x = platform.rect.x + platform.rect.width + width / 2;
                                    velocity.x = 0;
                                }
                            }
                        }
                        else if (platform.type == PASSTHROUGH) {
                            // Only top collision for passthrough platforms
                            if (stepY > 0) {
                                // Check if coming from above
                                if (playerRect.y + playerRect.height - stepY <= platform.rect.y) {
                                    position.y = platform.rect.y - height / 2;
                                    velocity.y = 0;
                                    onGround = true;

                                    // Check if air attack should cancel on landing
                                    if (currentAttack >= NEUTRAL_AIR && currentAttack <= DOWN_AIR) {
                                        resetAttackState();
                                        changeState(IDLE);
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
            attackFrame++;

            // End attack when duration is over and transition to appropriate state
            if (attackFrame >= attackDuration) {
                resetAttackState();

                // Return to appropriate state based on position and velocity
                if (onGround) {
                    if (fabs(velocity.x) > 0.5f) {
                        changeState(RUNNING);
                    } else {
                        changeState(IDLE);
                    }
                } else if (velocity.y < 0) {
                    changeState(JUMPING);
                } else {
                    changeState(FALLING);
                }
            }
            break;

        // Remaining states would be modified similarly...
        case SHIELDING:
            // Slowly regenerate shield
            shieldHealth = std::min(shieldHealth + SHIELD_REGEN_RATE, MAX_SHIELD_HEALTH);

            // No movement while shielding
            velocity.x = 0;
            velocity.y = 0;
            break;

        case DODGING:
            dodgeFrames++;

            // Check for invincibility frames
            if (dodgeFrames >= DODGE_INVINCIBLE_START && dodgeFrames <= DODGE_INVINCIBLE_END) {
                isInvincible = true;
            } else {
                isInvincible = false;
            }

            // End dodge after duration
            if (dodgeFrames >= SPOT_DODGE_FRAMES) {
                isDodging = false;
                dodgeFrames = 0;
                isInvincible = false;
                changeState(IDLE);
                dodgeCD.current = dodgeCD.duration; // Start cooldown
            }
            break;

        case HITSTUN:
            // Apply gravity
            velocity.y += GRAVITY;

            // Same collision logic as above
            // Setup sub-frame precision for collision detection
            stepX = velocity.x / collisionSteps;
            stepY = velocity.y / collisionSteps;

            for (int step = 0; step < collisionSteps; step++) {
                // Apply partial movement
                position.x += stepX;
                position.y += stepY;

                // Platform collision handling for hitstun state
                for (auto& platform : platforms) {
                    Rectangle playerRect = getRect();
                    if (CheckCollisionRecs(playerRect, platform.rect)) {
                        if (platform.type == SOLID) {
                            // Full collisions for solid platforms

                            // Top collision
                            if (stepY > 0) {
                                if (playerRect.y + playerRect.height > platform.rect.y &&
                                    playerRect.y + playerRect.height < platform.rect.y + platform.rect.height / 2) {
                                    position.y = platform.rect.y - height / 2;
                                    velocity.y = 0;
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
                                    position.x = platform.rect.x - width / 2;
                                    velocity.x = 0;
                                }
                                // Left side collision
                                else if (stepX < 0 &&
                                         playerRect.x < platform.rect.x + platform.rect.width &&
                                         playerRect.x + playerRect.width > platform.rect.x + platform.rect.width) {
                                    position.x = platform.rect.x + platform.rect.width + width / 2;
                                    velocity.x = 0;
                                }
                            }
                        }
                        else if (platform.type == PASSTHROUGH) {
                            // Only top collision for passthrough platforms
                            if (stepY > 0) {
                                // Check if coming from above
                                if (playerRect.y + playerRect.height - stepY <= platform.rect.y) {
                                    position.y = platform.rect.y - height / 2;
                                    velocity.y = 0;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // Decrement hitstun
            hitstunFrames--;
            if (hitstunFrames <= 0) {
                isHitstun = false;

                // Return to appropriate state
                if (velocity.y == 0) {
                    changeState(IDLE);
                } else if (velocity.y < 0) {
                    changeState(JUMPING);
                } else {
                    changeState(FALLING);
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

void Character::updateAttackPositions() {
    for (auto& attack : attacks) {
        // Position the attack box relative to the character
        float offsetX = isFacingRight ? 1.0f : -1.0f;
        float boxCenterX = position.x + (attack.rect.width / 2) * offsetX;

        // Adjust based on attack box original position
        attack.rect.x = boxCenterX - (attack.rect.width / 2);
        attack.rect.y = position.y - (attack.rect.height / 2);
    }
}

void Character::updateCooldowns() {
    // Update all cooldowns
    if (specialNeutralCD.current > 0) specialNeutralCD.current--;
    if (specialSideCD.current > 0) specialSideCD.current--;
    if (specialUpCD.current > 0) specialUpCD.current--;
    if (specialDownCD.current > 0) specialDownCD.current--;
    if (dodgeCD.current > 0) dodgeCD.current--;
}

void Character::changeState(CharacterState newState) {
    // Don't change state if already in hitstun
    if (state == HITSTUN && newState != DYING && hitstunFrames > 0) {
        return;
    }

    // Don't change state if currently attacking, unless hit or dying or attack is finished
    if (state == ATTACKING && newState != HITSTUN && newState != DYING &&
        attackFrame < attackDuration && isAttacking) {
        return;
    }

    // State change logic
    state = newState;

    // State-specific init
    switch (newState) {
        case IDLE:
            velocity.x *= 0.5f; // Slow down when returning to idle
            break;

        case JUMPING:
            // Animation reset
            break;

        case ATTACKING:
            // Attack init handled by specific attack methods
            break;

        case SHIELDING:
            velocity.x = 0;
            velocity.y = 0;
            break;

        case DODGING:
            dodgeFrames = 0;
            isDodging = true;
            break;

        case HITSTUN:
            // Handled by applyKnockback
            break;

        case DYING:
            // Handled by startDeathAnimation
            break;
    }
}

void Character::draw() {
    // Skip normal drawing if dying
    if (isDying) {
        drawDeathAnimation();
        return;
    }

    // Visual effects for states
    Color drawColor = color;

    // Flashing for invincibility
    if (isInvincible) {
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
        static_cast<int>(position.x - width/2),
        static_cast<int>(position.y - height/2),
        static_cast<int>(width),
        static_cast<int>(height),
        drawColor
    );

    // Direction indicator (eyes/face)
    float eyeOffset = isFacingRight ? width * 0.2f : -width * 0.2f;
    DrawCircle(
        static_cast<int>(position.x + eyeOffset),
        static_cast<int>(position.y - height * 0.1f),
        width * 0.15f,
        BLACK
    );

    // Shield visualization
    if (isShielding) {
        float shieldRatio = shieldHealth / MAX_SHIELD_HEALTH;
        float shieldSize = (width + height) * 0.4f * shieldRatio;
        Color shieldColor = {100, 200, 255, 128}; // Semi-transparent blue

        // Shield color shifts to red as it weakens
        shieldColor.g = static_cast<unsigned char>(200 * shieldRatio);
        shieldColor.b = static_cast<unsigned char>(255 * shieldRatio);

        DrawCircleV(position, shieldSize, shieldColor);
    }

    // Draw hitboxes if attacking (for debug)
    if (isAttacking) {
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
        static_cast<int>(position.x - width/2),
        static_cast<int>(position.y - height - 20),
        20,
        WHITE
    );

    // Animation counter
    framesCounter++;
}

void Character::startDeathAnimation() {
    if (!isDying) {
        isDying = true;
        state = DYING;
        deathFrame = 0;
        deathRotation = 0;
        deathScale = 1.0f;

        // Set initial death velocity based on current velocity
        deathVelocity = velocity;
        if (deathVelocity.y > -5.0f) deathVelocity.y = -5.0f; // Ensure upward motion

        // Store position at death
        deathPosition = position;

        // Reduce stock
        stocks--;
    }
}

void Character::updateDeathAnimation() {
    deathFrame++;

    // Update death position with velocity
    deathPosition.x += deathVelocity.x;
    deathPosition.y += deathVelocity.y;

    // Apply gravity to death animation
    deathVelocity.y += GRAVITY * 0.5f;

    // Spin and shrink
    deathRotation += 15.0f;
    deathScale = std::max(0.0f, 1.0f - static_cast<float>(deathFrame) / deathDuration);

    // End death animation
    if (deathFrame >= deathDuration) {
        isDying = false;

        if (stocks > 0) {
            // Reset for respawn
            damagePercent = 0;
            velocity = {0, 0};
            isInvincible = true;
            invincibilityFrames = 120; // 2 seconds of invincibility

            // Respawn at center top
            position.x = SCREEN_WIDTH / 2;
            position.y = 100;

            changeState(FALLING);
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
    if (deathFrame > deathDuration * 0.7f && deathFrame % 3 == 0) {
        float starAngle = static_cast<float>(GetRandomValue(0, 360));
        float starDist = static_cast<float>(GetRandomValue(10, 30));
        Vector2 starPos = {
            deathPosition.x + cosf(starAngle * DEG2RAD) * starDist,
            deathPosition.y + sinf(starAngle * DEG2RAD) * starDist
        };

        DrawCircleV(starPos, 5.0f * deathScale, WHITE);
    }
}

void Character::resetAttackState() {
    isAttacking = false;
    currentAttack = NONE;
    attackDuration = 0;
    attackFrame = 0;
    attacks.clear();
    canAttack = true;
}

void Character::respawn(Vector2 spawnPoint) {
    position = spawnPoint;
    velocity = {0, 0};
    damagePercent = 0;
    isInvincible = true;
    invincibilityFrames = 120; // 2 seconds of invincibility
    resetAttackState();
    changeState(FALLING);
}

// Movement methods
void Character::jump() {
    if (!isJumping && state != JUMPING) {
        velocity.y = JUMP_FORCE;
        isJumping = true;
        changeState(JUMPING);
    } else if (hasDoubleJump) {
        doubleJump();
    }
}

void Character::doubleJump() {
    if (hasDoubleJump) {
        velocity.y = DOUBLE_JUMP_FORCE;
        hasDoubleJump = false;
        changeState(JUMPING);
    }
}

void Character::moveLeft() {
    if (state != SHIELDING && state != DODGING) {
        velocity.x = -speed;
        isFacingRight = false;

        if (state == IDLE) {
            changeState(RUNNING);
        }
    }
}

void Character::moveRight() {
    if (state != SHIELDING && state != DODGING) {
        velocity.x = speed;
        isFacingRight = true;

        if (state == IDLE) {
            changeState(RUNNING);
        }
    }
}

void Character::fastFall() {
    if (velocity.y > 0) {
        isFastFalling = true;
        velocity.y = std::max(velocity.y, 5.0f); // Minimum fast fall speed
    }
}

// Defense methods
void Character::shield() {
    if (state != JUMPING && state != FALLING && !isAttacking &&
        shieldHealth > 0 && !isDodging) {
        changeState(SHIELDING);
        isShielding = true;

        // Immobilize the character
        velocity.x = 0;
        velocity.y = 0;

        // Shield shrinks as it takes damage
        // (Already implemented in draw function)
        }
}

void Character::releaseShield() {
    if (isShielding) {
        isShielding = false;
        changeState(IDLE);
    }
}

void Character::spotDodge() {
    if (state != JUMPING && state != FALLING && dodgeCD.current <= 0) {
        changeState(DODGING);
        dodgeFrames = 0;
        isDodging = true;

        // Stop movement during spot dodge
        velocity.x = 0;
        velocity.y = 0;

        // Shorter duration than rolls
        attackDuration = SPOT_DODGE_FRAMES;

        // Quick invincibility but can't move
    }
}

void Character::forwardDodge() {
    if (state != JUMPING && state != FALLING && dodgeCD.current <= 0) {
        changeState(DODGING);
        dodgeFrames = 0;
        isDodging = true;

        // Move in facing direction
        velocity.x = isFacingRight ? speed * 1.5f : -speed * 1.5f;
        velocity.y = 0;

        // Longer duration than spot dodge
        attackDuration = ROLL_DODGE_FRAMES;

        // Movement with invincibility, but fixed path and end lag
    }
}

void Character::backDodge() {
    if (state != JUMPING && state != FALLING && dodgeCD.current <= 0) {
        changeState(DODGING);
        dodgeFrames = 0;
        velocity.x = isFacingRight ? -speed * 1.5f : speed * 1.5f;
        velocity.y = 0;
    }
}

void Character::airDodge(float dirX, float dirY) {
    if ((state == JUMPING || state == FALLING) && dodgeCD.current <= 0) {
        changeState(DODGING);
        dodgeFrames = 0;
        isDodging = true;

        // Normalize direction and apply speed
        float length = sqrtf(dirX * dirX + dirY * dirY);
        if (length > 0) {
            velocity.x = (dirX / length) * speed * 1.5f;
            velocity.y = (dirY / length) * speed * 1.5f;
        }

        // Longest dodge with highest end lag
        attackDuration = AIR_DODGE_FRAMES;

        // Special landing penalty if air dodging
        // (would be implemented in the landing code)
    }
}


// Standard ground attacks
void Character::jab() {
    if (canAttack && state != JUMPING && state != FALLING) {
        resetAttackState();
        isAttacking = true;
        currentAttack = JAB;
        attackDuration = 15; // Very short duration
        changeState(ATTACKING);

        // Small hitbox with minimal knockback
        float hitboxWidth = width * 0.7f;
        float hitboxHeight = height * 0.5f;
        float hitboxX = isFacingRight ? position.x + width/2 : position.x - width/2 - hitboxWidth;
        float hitboxY = position.y - hitboxHeight/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(hitboxRect, 3.0f, 1.5f, 0.05f, isFacingRight ? 0.0f : 180.0f, 5, 5));

        // Reduced end lag - can act again sooner
        canAttack = true;
    }
}

void Character::forwardTilt() {
    if (canAttack && state != JUMPING && state != FALLING) {
        resetAttackState();
        isAttacking = true;
        currentAttack = FORWARD_TILT;
        attackDuration = 28; // Moderate duration
        changeState(ATTACKING);

        // Good range hitbox
        float hitboxWidth = width * 1.2f;
        float hitboxHeight = height * 0.6f;
        float hitboxX = isFacingRight ? position.x + width/2 : position.x - width/2 - hitboxWidth;
        float hitboxY = position.y - hitboxHeight/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(hitboxRect, 8.0f, 4.0f, 0.15f, isFacingRight ? 30.0f : 150.0f, 15, 12));
    }
}

void Character::downTilt() {
    if (canAttack && state != JUMPING && state != FALLING) {
        resetAttackState();
        isAttacking = true;
        currentAttack = DOWN_TILT;
        attackDuration = 22;
        changeState(ATTACKING);

        // Low-profile hitbox
        float hitboxWidth = width * 1.3f;
        float hitboxHeight = height * 0.3f; // Very low profile
        float hitboxX = isFacingRight ? position.x + width/2 : position.x - width/2 - hitboxWidth;
        float hitboxY = position.y + height/2 - hitboxHeight;

        // More horizontal launch angle - good for combos
        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(hitboxRect, 5.0f, 2.5f, 0.1f, isFacingRight ? 15.0f : 165.0f, 10, 8));

        // Add slight movement to help with combos
        velocity.x = isFacingRight ? speed * 0.3f : -speed * 0.3f;
    }
}

void Character::upTilt() {
    if (canAttack && state != JUMPING && state != FALLING) {
        resetAttackState();
        isAttacking = true;
        currentAttack = UP_TILT;
        attackDuration = 25;
        changeState(ATTACKING);

        // Tall, narrow hitbox above character
        float hitboxWidth = width * 0.7f;
        float hitboxHeight = height * 1.3f;
        float hitboxX = position.x - hitboxWidth/2;
        float hitboxY = position.y - hitboxHeight;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(hitboxRect, 7.0f, 3.0f, 0.15f, 80.0f, 12, 12));

        // Brief vulnerability on sides during animation
        // (implemented through narrow hitbox and animation duration)
    }
}

void Character::dashAttack() {
    if (canAttack && (state == RUNNING)) {
        resetAttackState();
        isAttacking = true;
        currentAttack = DASH_ATTACK;
        attackDuration = 35; // Longer commitment
        changeState(ATTACKING);

        // Add significant momentum
        velocity.x = isFacingRight ? speed * 2.0f : -speed * 2.0f;

        // Forward-moving hitbox
        float hitboxWidth = width * 1.1f;
        float hitboxHeight = height * 0.8f;
        float hitboxX = isFacingRight ? position.x + width/2 : position.x - width/2 - hitboxWidth;
        float hitboxY = position.y - hitboxHeight/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(hitboxRect, 10.0f, 5.0f, 0.15f, isFacingRight ? 40.0f : 140.0f, 20, 20));

        // High endlag - vulnerable if missed or shielded
        canAttack = false; // Force full animation before next action
    }
}

// Smash attacks
void Character::forwardSmash(float chargeTime) {
    if (canAttack && state != JUMPING && state != FALLING) {
        resetAttackState();
        isAttacking = true;
        currentAttack = FORWARD_SMASH;
        attackDuration = 45; // Very high commitment
        changeState(ATTACKING);

        // Charge multiplier (1.0 to 1.5)
        float chargeMultiplier = 1.0f + std::min(chargeTime / 60.0f, 0.5f);

        // Large, strong forward hitbox
        float hitboxWidth = width * 1.5f;
        float hitboxHeight = height * 0.7f;
        float hitboxX = isFacingRight ? position.x + width/2 : position.x - width/2 - hitboxWidth;
        float hitboxY = position.y - hitboxHeight/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};

        // High knockback, scaling with charge
        attacks.push_back(AttackBox(
            hitboxRect,
            15.0f * chargeMultiplier,
            8.0f * chargeMultiplier,
            0.3f,
            isFacingRight ? 35.0f : 145.0f,
            25,
            15
        ));

        // Vulnerable during endlag - major disadvantage if missed
        velocity.x *= 0.2f; // Almost stop movement - committal
    }
}

void Character::upSmash(float chargeTime) {
    if (canAttack && state != JUMPING && state != FALLING) {
        resetAttackState();
        isAttacking = true;
        currentAttack = UP_SMASH;
        attackDuration = 40;
        changeState(ATTACKING);

        // Charge multiplier (1.0 to 1.5)
        float chargeMultiplier = 1.0f + std::min(chargeTime / 60.0f, 0.5f);

        // Tall vertical hitbox
        float hitboxWidth = width * 0.8f;
        float hitboxHeight = height * 1.8f;
        float hitboxX = position.x - hitboxWidth/2;
        float hitboxY = position.y - hitboxHeight;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};

        // Strong vertical knockback
        attacks.push_back(AttackBox(
            hitboxRect,
            14.0f * chargeMultiplier,
            7.0f * chargeMultiplier,
            0.35f,
            90.0f,
            20,
            15
        ));

        // Vulnerable on sides - blind spot weakness
    }
}

void Character::downSmash(float chargeTime) {
    if (canAttack && state != JUMPING && state != FALLING) {
        resetAttackState();
        isAttacking = true;
        currentAttack = DOWN_SMASH;
        attackDuration = 38;
        changeState(ATTACKING);

        // Charge multiplier (1.0 to 1.5)
        float chargeMultiplier = 1.0f + std::min(chargeTime / 60.0f, 0.5f);

        // Two hitboxes on both sides - slightly less range than fsmash
        float hitboxWidth = width * 0.9f;
        float hitboxHeight = height * 0.5f;
        float hitboxY = position.y + height/2 - hitboxHeight/2;

        // Left hitbox
        Rectangle leftHitboxRect = {position.x - width/2 - hitboxWidth, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(
            leftHitboxRect,
            13.0f * chargeMultiplier,
            6.0f * chargeMultiplier,
            0.3f,
            20.0f, // Semi-spike angle
            20,
            15
        ));

        // Right hitbox
        Rectangle rightHitboxRect = {position.x + width/2, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(
            rightHitboxRect,
            13.0f * chargeMultiplier,
            6.0f * chargeMultiplier,
            0.3f,
            160.0f, // Semi-spike angle
            20,
            15
        ));

        // Solid endlag but less than the other smashes
        attackDuration = 35;
    }
}

// Aerial attacks
void Character::neutralAir() {
    if (canAttack && (state == JUMPING || state == FALLING)) {
        resetAttackState();
        isAttacking = true;
        currentAttack = NEUTRAL_AIR;
        attackDuration = 25;
        changeState(ATTACKING);

        // Circle hitbox around character - hits all around
        float hitboxRadius = width * 1.2f;
        Rectangle hitboxRect = {
            position.x - hitboxRadius/2,
            position.y - hitboxRadius/2,
            hitboxRadius,
            hitboxRadius
        };

        // Moderate damage, low knockback, faster than other aerials
        attacks.push_back(AttackBox(hitboxRect, 8.0f, 3.0f, 0.12f, 45.0f, 15, 12));

        // Low landing lag if attack has finished
        // (would be implemented in the land function)
    }
}

void Character::forwardAir() {
    if (canAttack && (state == JUMPING || state == FALLING)) {
        resetAttackState();
        isAttacking = true;
        currentAttack = FORWARD_AIR;
        attackDuration = 30;
        changeState(ATTACKING);

        // Forward-reaching hitbox
        float hitboxWidth = width * 1.3f;
        float hitboxHeight = height * 0.7f;
        float hitboxX = isFacingRight ? position.x + width/2 : position.x - width/2 - hitboxWidth;
        float hitboxY = position.y - hitboxHeight/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};

        // Good damage and knockback
        attacks.push_back(AttackBox(hitboxRect, 10.0f, 4.5f, 0.2f, isFacingRight ? 45.0f : 135.0f, 20, 15));

        // Moderate landing lag penalty if not completed in air
    }
}

void Character::backAir() {
    if (canAttack && (state == JUMPING || state == FALLING)) {
        resetAttackState();
        isAttacking = true;
        currentAttack = BACK_AIR;
        attackDuration = 28;
        changeState(ATTACKING);

        // Back hitbox (opposite of facing direction)
        float hitboxWidth = width * 1.1f;
        float hitboxHeight = height * 0.8f;
        float hitboxX = isFacingRight ? position.x - width/2 - hitboxWidth : position.x + width/2;
        float hitboxY = position.y - hitboxHeight/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};

        // Strong knockback - kill move
        attacks.push_back(AttackBox(hitboxRect, 13.0f, 6.0f, 0.25f, isFacingRight ? 135.0f : 45.0f, 25, 15));

        // High landing lag if not completed
    }
}

void Character::upAir() {
    if (canAttack && (state == JUMPING || state == FALLING)) {
        resetAttackState();
        isAttacking = true;
        currentAttack = UP_AIR;
        attackDuration = 25;
        changeState(ATTACKING);

        // Upward hitbox
        float hitboxWidth = width * 0.8f;
        float hitboxHeight = height * 1.1f;
        float hitboxX = position.x - hitboxWidth/2;
        float hitboxY = position.y - height/2 - hitboxHeight;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};

        // Moderate damage, upward launch angle
        attacks.push_back(AttackBox(hitboxRect, 9.0f, 4.0f, 0.2f, 85.0f, 15, 12));

        // Medium landing lag
        // Stronger when hit at the apex of the attack
    }
}

void Character::downAir() {
    if (canAttack && (state == JUMPING || state == FALLING)) {
        resetAttackState();
        isAttacking = true;
        currentAttack = DOWN_AIR;
        attackDuration = 35; // Longest aerial
        changeState(ATTACKING);

        // Downward hitbox
        float hitboxWidth = width * 0.7f;
        float hitboxHeight = height * 1.0f;
        float hitboxX = position.x - hitboxWidth/2;
        float hitboxY = position.y + height/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};

        // Strong spike with high risk/reward
        AttackBox spike(hitboxRect, 14.0f, 3.0f, 0.15f, 270.0f, 25, 20);
        spike.canSpike = true;
        attacks.push_back(spike);

        // Very high landing lag if not completed
        // Fast fall when using for maximum impact
        isFastFalling = true;
    }
}

// Special attacks
void Character::neutralSpecial() {
    if (canAttack && specialNeutralCD.current <= 0) {
        resetAttackState();
        isAttacking = true;
        currentAttack = NEUTRAL_SPECIAL;
        attackDuration = 40;
        specialNeutralCD.current = specialNeutralCD.duration * 0.8f; // 80% of full cooldown
        changeState(ATTACKING);

        // Projectile hitbox
        float hitboxWidth = width * 0.8f;
        float hitboxHeight = height * 0.6f;
        float hitboxX = isFacingRight ? position.x + width/2 : position.x - width/2 - hitboxWidth;
        float hitboxY = position.y - hitboxHeight/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};

        // Medium-speed projectile with moderate damage
        Vector2 projectileVel = {isFacingRight ? 8.0f : -8.0f, 0};
        attacks.push_back(AttackBox(
            hitboxRect, 8.0f, 2.0f, 0.1f, isFacingRight ? 0.0f : 180.0f, 15, 90,
            projectileVel, true
        ));

        // Brief immobility during startup frames
        velocity.x *= 0.3f;
    }
}

void Character::sideSpecial() {
    if (canAttack && specialSideCD.current <= 0) {
        resetAttackState();
        isAttacking = true;
        currentAttack = SIDE_SPECIAL;
        attackDuration = 45;
        specialSideCD.current = specialSideCD.duration;
        changeState(ATTACKING);

        // Add significant horizontal boost
        velocity.x = isFacingRight ? speed * 2.5f : -speed * 2.5f;

        // Side special hitbox
        float hitboxWidth = width * 1.5f;
        float hitboxHeight = height * 0.9f;
        float hitboxX = isFacingRight ? position.x + width/2 : position.x - width/2 - hitboxWidth;
        float hitboxY = position.y - hitboxHeight/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(hitboxRect, 12.0f, 6.0f, 0.25f, isFacingRight ? 30.0f : 150.0f, 25, 25));

        // Vulnerable during cooldown - can be punished if shielded
    }
}

void Character::upSpecial() {
    if (canAttack && specialUpCD.current <= 0) {
        resetAttackState();
        isAttacking = true;
        currentAttack = UP_SPECIAL;
        attackDuration = 40;
        specialUpCD.current = specialUpCD.duration;
        changeState(ATTACKING);

        // Strong vertical boost with slight horizontal component
        velocity.y = JUMP_FORCE * 1.4f;
        velocity.x = isFacingRight ? speed * 0.5f : -speed * 0.5f;

        // Brief invincibility at startup (5 frames)
        isInvincible = true;
        invincibilityFrames = 5;

        // Up special hitbox
        float hitboxWidth = width * 1.2f;
        float hitboxHeight = height * 1.5f;
        float hitboxX = position.x - hitboxWidth/2;
        float hitboxY = position.y - hitboxHeight;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};
        attacks.push_back(AttackBox(hitboxRect, 10.0f, 5.0f, 0.2f, 80.0f, 20, 20));

        // High vulnerability after recovery frames - cooldown
    }
}

void Character::downSpecial() {
    if (canAttack && specialDownCD.current <= 0) {
        resetAttackState();
        isAttacking = true;
        currentAttack = DOWN_SPECIAL;
        attackDuration = 45;
        specialDownCD.current = specialDownCD.duration * 1.2f; // 120% cooldown for powerful move
        changeState(ATTACKING);

        // Counter/reflector hitbox
        float hitboxWidth = width * 1.5f;
        float hitboxHeight = height * 1.5f;
        float hitboxX = position.x - hitboxWidth/2;
        float hitboxY = position.y - hitboxHeight/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};

        // Create a reflector hitbox with counter properties
        AttackBox reflector(hitboxRect, 6.0f, 3.0f, 0.1f, 45.0f, 15, 30, AttackBox::REFLECTOR);
        attacks.push_back(reflector);

        // High commitment - vulnerable if missed
        // Freeze in place while countering
        velocity.x = 0;
        velocity.y *= 0.3f; // Reduced falling during counter
    }
}

// Grab and throws
void Character::grab() {
    if (canAttack && !isGrabbing && (state != JUMPING && state != FALLING)) {
        resetAttackState();
        isAttacking = true;
        currentAttack = GRAB;
        attackDuration = 25;
        changeState(ATTACKING);

        // Grab hitbox
        float hitboxWidth = width * 0.8f;
        float hitboxHeight = height * 0.7f;
        float hitboxX = isFacingRight ? position.x + width/2 : position.x - width/2 - hitboxWidth;
        float hitboxY = position.y - hitboxHeight/2;

        Rectangle hitboxRect = {hitboxX, hitboxY, hitboxWidth, hitboxHeight};

        // Grab hitbox - no damage but initiates grab state
        AttackBox grabBox(hitboxRect, 0.0f, 0.0f, 0.0f, 0.0f, 0, 10, AttackBox::GRAB);
        attacks.push_back(grabBox);

        // High end lag if missed - very punishable
    }
}

void Character::pummel() {
    if (isGrabbing && grabbedCharacter != nullptr) {
        // Add damage to grabbed character
        grabbedCharacter->applyDamage(2.0f);

        // Create hit effect at grabbed character position
        createHitEffect(grabbedCharacter->position);

        // Reset grab duration to extend hold
        grabFrame = 0;
    }
}

void Character::forwardThrow() {
    if (isGrabbing && grabbedCharacter != nullptr) {
        // Apply damage and knockback
        grabbedCharacter->applyDamage(8.0f);
        grabbedCharacter->applyKnockback(8.0f, 5.0f, 0.2f, isFacingRight ? 1.0f : -1.0f, 0.2f);

        // Create hit effect
        createHitEffect(grabbedCharacter->position);

        // Release grab
        releaseGrab();
    }
}

void Character::backThrow() {
    if (isGrabbing && grabbedCharacter != nullptr) {
        // Apply damage and knockback
        grabbedCharacter->applyDamage(10.0f);
        grabbedCharacter->applyKnockback(10.0f, 6.0f, 0.25f, isFacingRight ? -1.0f : 1.0f, 0.2f);

        // Create hit effect
        createHitEffect(grabbedCharacter->position);

        // Release grab
        releaseGrab();
    }
}

void Character::upThrow() {
    if (isGrabbing && grabbedCharacter != nullptr) {
        // Apply damage and knockback
        grabbedCharacter->applyDamage(9.0f);
        grabbedCharacter->applyKnockback(9.0f, 5.0f, 0.22f, 0.0f, -1.0f);

        // Create hit effect
        createHitEffect(grabbedCharacter->position);

        // Release grab
        releaseGrab();
    }
}

void Character::downThrow() {
    if (isGrabbing && grabbedCharacter != nullptr) {
        // Apply damage and knockback
        grabbedCharacter->applyDamage(7.0f);
        grabbedCharacter->applyKnockback(7.0f, 3.0f, 0.15f, isFacingRight ? 0.5f : -0.5f, 0.8f);

        // Create hit effect
        createHitEffect(grabbedCharacter->position);

        // Release grab
        releaseGrab();
    }
}

void Character::releaseGrab() {
    if (isGrabbing && grabbedCharacter != nullptr) {
        isGrabbing = false;
        grabbedCharacter = nullptr;
        grabDuration = 0;
        grabFrame = 0;

        // Return to idle state
        changeState(IDLE);
    }
}

// Collision and damage
bool Character::checkHit(Character& other) {
    // Skip if the other character is invincible or dying
    if (other.isInvincible || other.isDying) return false;

    // Check each attack hitbox
    for (auto& attack : attacks) {
        Rectangle otherHurtbox = other.getHurtbox();

        if (CheckCollisionRecs(attack.rect, otherHurtbox)) {
            // Handle different hitbox types
            switch (attack.type) {
                case AttackBox::GRAB:
                    // Initiate grab
                    if (!other.isShielding) {
                        isGrabbing = true;
                        grabbedCharacter = &other;
                        grabDuration = 120; // Hold for 2 seconds max
                        grabFrame = 0;

                        // Position the grabbed character
                        float grabOffset = isFacingRight ? width : -width;
                        other.position.x = position.x + grabOffset;
                        other.position.y = position.y;

                        other.velocity = {0, 0};
                        other.isHitstun = true;
                        other.hitstunFrames = 1; // Keep in hitstun while grabbed
                    }
                    break;

                case AttackBox::NORMAL:
                default:
                    // Handle shield
                    if (other.isShielding) {
                        // Reduce shield health
                        other.shieldHealth -= attack.damage * SHIELD_DAMAGE_MULTIPLIER;

                        // Shield break
                        if (other.shieldHealth <= 0) {
                            other.shieldHealth = 0;
                            other.isShielding = false;
                            other.isHitstun = true;
                            other.hitstunFrames = SHIELD_BREAK_STUN;

                            // Apply upward knockback
                            other.velocity.y = -8.0f;
                        }

                        // Shield stun
                        other.isHitstun = true;
                        other.hitstunFrames = SHIELD_STUN_FRAMES + attack.shieldStun;
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
    // Add damage percentage, capped at maximum
    damagePercent = std::min(damagePercent + damage, MAX_DAMAGE);
}

void Character::applyKnockback(float damage, float baseKnockback, float knockbackScaling, float directionX, float directionY) {
    // Calculate knockback magnitude (Smash-style formula)
    float knockbackMagnitude = baseKnockback + (damage * damagePercent * DAMAGE_SCALING * knockbackScaling);

    // Apply knockback vector
    velocity.x = directionX * knockbackMagnitude;
    velocity.y = directionY * knockbackMagnitude;

    // Apply hitstun based on knockback
    int hitstunAmount = static_cast<int>(knockbackMagnitude * HITSTUN_MULTIPLIER);
    isHitstun = true;
    hitstunFrames = hitstunAmount;

    // Change state
    changeState(HITSTUN);

    // Cap maximum knockback to prevent phasing through floors
    const float MAX_KNOCKBACK_Y = 20.0f;
    if (velocity.y > MAX_KNOCKBACK_Y) velocity.y = MAX_KNOCKBACK_Y;
    if (velocity.y < -MAX_KNOCKBACK_Y) velocity.y = -MAX_KNOCKBACK_Y;
}

void Character::createHitEffect(Vector2 position) {
    // Create hit effect at position
    hitEffects.push_back(HitEffect(position, WHITE));
}

bool Character::isOutOfBounds() {
    return position.x < BLAST_ZONE_LEFT ||
           position.x > BLAST_ZONE_RIGHT ||
           position.y < BLAST_ZONE_TOP ||
           position.y > BLAST_ZONE_BOTTOM;
}

void Character::dropThroughPlatform() {
    // Check if character is standing on a platform
    if (state == IDLE || state == RUNNING) {
        // Move character down slightly to avoid immediate re-collision
        position.y += 5;

        // Apply a small downward velocity to ensure continued falling
        velocity.y = 1.0f;

        // Change state to falling
        changeState(FALLING);
    }
}