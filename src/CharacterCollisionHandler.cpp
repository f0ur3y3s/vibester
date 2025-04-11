#include "CharacterCollisionHandler.h"
#include "character/Character.h"
#include <cmath>
#include <algorithm>

bool CharacterCollisionHandler::handlePlatformCollisions(std::vector<Platform>& platforms) {
    bool onGround = false;

    // Constants for collision detection
    const int collisionSteps = 4;  // Number of sub-steps for collision checking
    float stepX = character.physics.velocity.x / collisionSteps;
    float stepY = character.physics.velocity.y / collisionSteps;

    // Original position to restore if needed
    Vector2 originalPosition = character.physics.position;

    // Apply movement in steps for better collision detection
    for (int step = 0; step < collisionSteps; step++) {
        // Apply partial movement
        character.physics.updatePositionPartial(stepX, stepY);

        // Check collisions with all platforms
        for (auto& platform : platforms) {
            Rectangle playerRect = character.getRect();

            if (CheckCollisionRecs(playerRect, platform.rect)) {
                bool collisionHandled = checkPlatformCollision(platform, playerRect, stepX, stepY);

                // If we hit the top of a platform, mark as on ground
                if (collisionHandled && fabs(character.physics.velocity.y) < 0.1f && stepY > 0) {
                    onGround = true;

                    // Reset states that need ground
                    if (character.stateManager.isJumping) {
                        character.stateManager.isJumping = false;
                    }
                    character.stateManager.hasDoubleJump = true;
                    character.stateManager.isHitstun = false;
                }
            }
        }
    }

    return onGround;
}

bool CharacterCollisionHandler::checkPlatformCollision(const Platform& platform,
                                                     Rectangle playerRect,
                                                     float stepX, float stepY) {
    switch (platform.type) {
        case SOLID:
            return handleSolidPlatform(platform, playerRect, stepX, stepY);

        case PASSTHROUGH:
            return handlePassthroughPlatform(platform, playerRect, stepX, stepY);

        default:
            return false;
    }
}

bool CharacterCollisionHandler::handleSolidPlatform(const Platform& platform,
                                                  Rectangle playerRect,
                                                  float stepX, float stepY) {
    bool collisionHandled = false;

    // Top collision - only check if moving downward
    if (stepY > 0) {
        if (playerRect.y + playerRect.height > platform.rect.y &&
            playerRect.y + playerRect.height < platform.rect.y + platform.rect.height / 2) {
            // Coming from above - land on platform
            character.physics.position.y = platform.rect.y - character.height / 2;
            character.physics.velocity.y = 0;
            collisionHandled = true;

            // Check if air attack should cancel on landing
            if (character.stateManager.isAttacking &&
                character.stateManager.currentAttack >= NEUTRAL_AIR &&
                character.stateManager.currentAttack <= DOWN_AIR) {
                character.resetAttackState();
                character.stateManager.changeState(IDLE);
            }

            return true;
        }
    }

    // Only apply side collision if player's feet are below platform top
    // AND player's top is not above platform bottom (to allow movement under platforms)
    if (playerRect.y + playerRect.height > platform.rect.y + 5 &&
        playerRect.y < platform.rect.y + platform.rect.height) {

        // Right side collision - player moving right into platform left edge
        if (stepX > 0 &&
            playerRect.x + playerRect.width > platform.rect.x &&
            playerRect.x < platform.rect.x) {
            character.physics.position.x = platform.rect.x - character.width / 2;
            character.physics.velocity.x = 0;
            collisionHandled = true;
        }
        // Left side collision - player moving left into platform right edge
        else if (stepX < 0 &&
                 playerRect.x < platform.rect.x + platform.rect.width &&
                 playerRect.x + playerRect.width > platform.rect.x + platform.rect.width) {
            character.physics.position.x = platform.rect.x + platform.rect.width + character.width / 2;
            character.physics.velocity.x = 0;
            collisionHandled = true;
        }
    }

    return collisionHandled;
}

bool CharacterCollisionHandler::handlePassthroughPlatform(const Platform& platform,
                                                        Rectangle playerRect,
                                                        float stepX, float stepY) {
    // PASSTHROUGH platforms only have collision from above
    if (stepY > 0) {
        // Previous position was above the platform
        float prevBottom = playerRect.y + playerRect.height - stepY;
        if (prevBottom <= platform.rect.y) {
            character.physics.position.y = platform.rect.y - character.height / 2;
            character.physics.velocity.y = 0;

            // Reset states that need ground
            if (character.stateManager.isJumping) {
                character.stateManager.isJumping = false;
            }
            character.stateManager.hasDoubleJump = true;
            character.stateManager.isHitstun = false;

            // Check if air attack should cancel on landing
            if (character.stateManager.isAttacking &&
                character.stateManager.currentAttack >= NEUTRAL_AIR &&
                character.stateManager.currentAttack <= DOWN_AIR) {
                character.resetAttackState();
                character.stateManager.changeState(IDLE);
            }

            return true;
        }
    }

    // No side or bottom collisions for passthrough platforms
    return false;
}

bool CharacterCollisionHandler::checkBlastZoneCollision() {
    // Check if character is outside blast zones
    if (character.physics.position.x < GameConfig::BLAST_ZONE_LEFT ||
        character.physics.position.x > GameConfig::BLAST_ZONE_RIGHT ||
        character.physics.position.y < GameConfig::BLAST_ZONE_TOP ||
        character.physics.position.y > GameConfig::BLAST_ZONE_BOTTOM) {

        // Character is out of bounds - start death animation
        character.startDeathAnimation();
        return true;
    }

    return false;
}

bool CharacterCollisionHandler::checkAttackCollision(Character& other) {
    // Skip if the other character is invincible or dying
    if (other.stateManager.isInvincible ||
        other.stateManager.isDying ||
        !character.stateManager.isAttacking) {
        return false;
    }

    // Check each attack hitbox
    for (auto it = character.attacks.begin(); it != character.attacks.end(); ) {
        auto& attack = *it;
        
        // Skip inactive attacks
        if (!attack.isActive) {
            ++it;
            continue;
        }
        
        Rectangle otherHurtbox = other.getHurtbox();

        if (CheckCollisionRecs(attack.rect, otherHurtbox)) {
            // Handle different hitbox types
            switch (attack.type) {
                case AttackBox::GRAB:
                    // Initiate grab if not shielding
                    if (!other.stateManager.isShielding) {
                        character.stateManager.isGrabbing = true;
                        character.grabbedCharacter = &other;
                        character.stateManager.grabDuration = 120; // 2 seconds max
                        character.stateManager.grabFrame = 0;

                        // Position the grabbed character
                        float grabOffset = character.stateManager.isFacingRight ? character.width : -character.width;
                        other.physics.position.x = character.physics.position.x + grabOffset;
                        other.physics.position.y = character.physics.position.y;

                        other.physics.velocity = {0, 0};
                        other.stateManager.isHitstun = true;
                        other.stateManager.hitstunFrames = 1; // Keep in hitstun while grabbed

                        return true;
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
                        } else {
                            // Shield stun
                            other.stateManager.isHitstun = true;
                            other.stateManager.hitstunFrames = GameConfig::SHIELD_STUN_FRAMES + attack.shieldStun;
                        }
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
                        character.createHitEffect(hitPos);
                    }

                    // If this is a projectile that should be destroyed on hit
                    if (attack.type == AttackBox::PROJECTILE && attack.destroyOnHit) {
                        attack.isActive = false;
                        // Remove the projectile from the attack list
                        it = character.attacks.erase(it);
                        return true;
                    }

                    // For non-projectile attacks, increment iterator
                    ++it;
                    return true;
            }
        }
    }

    return false;
}

bool CharacterCollisionHandler::isOnGround(const std::vector<Platform>& platforms) {
    // Create a small ray below the character to check for ground
    Rectangle playerRect = character.getRect();
    Rectangle groundCheckRect = {
        playerRect.x,
        playerRect.y + playerRect.height,
        playerRect.width,
        5.0f  // Small distance below feet
    };

    for (const auto& platform : platforms) {
        if (CheckCollisionRecs(groundCheckRect, platform.rect)) {
            // For passthrough platforms, only count as ground if character is on top
            if (platform.type == PASSTHROUGH) {
                if (playerRect.y + playerRect.height <= platform.rect.y + 2.0f) {
                    return true;
                }
            } else {
                // For solid platforms, any collision with the ground check counts
                return true;
            }
        }
    }

    return false;
}