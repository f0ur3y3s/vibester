#include "Character.h"
#include "Constants.h"
#include <cmath>
#include <algorithm>

Character::Character(float x, float y, float w, float h, float spd, Color col, std::string n) :
    position({x, y}), width(w), height(h), speed(spd), isJumping(false),
    isFacingRight(true), isAttacking(false), damage(0), color(col), name(n),
    velocity({0, 0}), currentFrame(0), framesCounter(0), framesSpeed(8),
    isSpecialAttack(false), currentAttack(NONE), attackDuration(0), attackFrame(0),
    canAttack(true), specialCooldown(60), currentCooldown(0),
    specialSideCooldown(45), currentSideCooldown(0),
    specialUpCooldown(90), currentUpCooldown(0),
    specialDownCooldown(70), currentDownCooldown(0),
    isRecovering(false), isDying(false), deathRotation(0), deathScale(1.0f),
    deathDuration(60), deathFrame(0), deathVelocity({0, 0}), deathPosition({0, 0}) {}

Rectangle Character::getRect() {
    return {position.x - width/2, position.y - height, width, height};
}

void Character::update(std::vector<Platform>& platforms) {
    // If dying, handle death animation
    if (isDying) {
        updateDeathAnimation();
        return;
    }
    
    // Apply gravity if not recovering with up-B
    if (!isRecovering) {
        velocity.y += GRAVITY;
    }
    
    // Apply velocity
    position.x += velocity.x;
    position.y += velocity.y;
    
    // Handle platform collisions
    isJumping = true;
    for (auto& platform : platforms) {
        Rectangle charRect = getRect();
        
        if (CheckCollisionRecs(charRect, platform.rect)) {
            // Check if landing on top of platform
            if (velocity.y > 0 && charRect.y + charRect.height - velocity.y <= platform.rect.y) {
                position.y = platform.rect.y;
                velocity.y = 0;
                isJumping = false;
                isRecovering = false; // End recovery when landing
            }
            // Side or bottom collision - push out
            else {
                if (velocity.x > 0) position.x = platform.rect.x - width/2;
                else if (velocity.x < 0) position.x = platform.rect.x + platform.rect.width + width/2;
                velocity.x = 0;
            }
        }
    }
    
    // Screen boundaries
    if (position.x - width/2 < 0) {
        position.x = width/2;
        velocity.x = 0;
    }
    if (position.x + width/2 > SCREEN_WIDTH) {
        position.x = SCREEN_WIDTH - width/2;
        velocity.x = 0;
    }
    
    // Check if character fell off screen - start death animation
    if (position.y > SCREEN_HEIGHT + 100) {
        startDeathAnimation();
    }
    
    // Update attacks
    attacks.erase(
        std::remove_if(attacks.begin(), attacks.end(), [](AttackBox& a) { 
            return !a.update(); 
        }),
        attacks.end()
    );
    
    // Update attack hitbox positions to follow character
    updateAttackPositions();
    
    // Update cooldowns
    if (currentCooldown > 0) currentCooldown--;
    if (currentSideCooldown > 0) currentSideCooldown--;
    if (currentUpCooldown > 0) currentUpCooldown--;
    if (currentDownCooldown > 0) currentDownCooldown--;
    
    // Update attack state
    if (isAttacking) {
        attackFrame++;
        
        // Check if attack is finished
        if (attackFrame >= attackDuration) {
            resetAttackState();
        }
        // For recovery move, end the recovery state after a certain duration
        else if (currentAttack == SPECIAL_UP && attackFrame > attackDuration / 2) {
            isRecovering = false;
        }
    }
    
    // Animation logic
    framesCounter++;
    if (framesCounter >= 60/framesSpeed) {
        framesCounter = 0;
        currentFrame++;
        if (currentFrame > 3) currentFrame = 0;
    }
    
    // Damping velocity for smoother movement
    if (!isRecovering) {
        velocity.x *= 0.9f;
        if (std::abs(velocity.x) < 0.1f) velocity.x = 0;
    }
}

void Character::updateAttackPositions() {
    // Update attack hitboxes positions to follow the character
    for (auto& attack : attacks) {
        // Only update position for active attacks that belong to current attack type
        if (attack.currentFrame < attack.duration / 2) {
            switch (currentAttack) {
                case NEUTRAL:
                    {
                        float attackWidth = width * 1.2f;
                        float attackX = isFacingRight ? position.x + width*0.3f : position.x - width*0.3f - attackWidth;
                        attack.rect.x = attackX;
                        attack.rect.y = position.y - height * 0.8f;
                    }
                    break;
                case SIDE:
                    {
                        float attackWidth = width * 1.5f;
                        float attackX = isFacingRight ? position.x + width*0.4f : position.x - width*0.4f - attackWidth;
                        attack.rect.x = attackX;
                        attack.rect.y = position.y - height * 0.7f;
                    }
                    break;
                case UP:
                    {
                        attack.rect.x = position.x - width * 0.7f;
                        attack.rect.y = position.y - height * 1.5f;
                    }
                    break;
                case DOWN:
                    {
                        attack.rect.x = position.x - width;
                        attack.rect.y = position.y;
                    }
                    break;
                case SPECIAL_NEUTRAL:
                    {
                        float attackWidth = width * 3.0f;
                        float attackX = isFacingRight ? position.x + width/2 : position.x - width/2 - attackWidth;
                        attack.rect.x = attackX;
                        attack.rect.y = position.y - height * 0.6f;
                    }
                    break;
                case SPECIAL_SIDE:
                    {
                        attack.rect.x = position.x - width;
                        attack.rect.y = position.y - height;
                    }
                    break;
                case SPECIAL_UP:
                    {
                        attack.rect.x = position.x - width/2;
                        attack.rect.y = position.y;
                    }
                    break;
                case SPECIAL_DOWN:
                    {
                        attack.rect.x = position.x - width * 1.5f;
                        attack.rect.y = position.y - height * 1.5f;
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void Character::startDeathAnimation() {
    isDying = true;
    deathFrame = 0;
    deathRotation = 0;
    deathScale = 1.0f;
    deathPosition = position;
    
    // Set initial blast-off velocity (upward and away from center)
    float directionX = (deathPosition.x < SCREEN_WIDTH / 2) ? -5.0f : 5.0f;
    deathVelocity = {directionX, -15.0f};
    
    // Clear any attacks
    attacks.clear();
    resetAttackState();
}

void Character::updateDeathAnimation() {
    deathFrame++;
    
    // Update death position
    deathVelocity.y += 0.2f; // Lighter gravity for dramatic effect
    deathPosition.x += deathVelocity.x;
    deathPosition.y += deathVelocity.y;
    
    // Update rotation and scale
    deathRotation += 15.0f; // Spin speed
    deathScale -= 0.01f;
    if (deathScale < 0.01f) deathScale = 0.01f;
    
    // Check if death animation is complete
    if (deathFrame >= deathDuration) {
        // Reset character
        isDying = false;
        position.x = SCREEN_WIDTH / 2.0f;
        position.y = 100.0f;
        velocity.x = 0.0f;
        velocity.y = 0.0f;
        damage = 0;
        deathScale = 1.0f;
        deathRotation = 0;
    }
}

void Character::resetAttackState() {
    isAttacking = false;
    isSpecialAttack = false;
    attackFrame = 0;
    currentAttack = NONE;
    canAttack = true;
    
    // We don't want to reset the position when ending an attack
    // No position resetting should happen here
}

void Character::draw() {
    if (isDying) {
        drawDeathAnimation();
        return;
    }
    
    Rectangle charRect = getRect();
    
    // Draw character
    DrawRectangleRec(charRect, color);
    
    // Draw toilet bowl features if this is "The Throne"
    if (name == "The Throne") {
        // Draw toilet base
        DrawRectangleRounded({charRect.x, charRect.y + charRect.height * 0.6f, charRect.width, charRect.height * 0.4f}, 0.5f, 10, LIGHTGRAY);
        
        // Draw toilet seat
        DrawRectangleRounded({charRect.x, charRect.y, charRect.width, charRect.height * 0.2f}, 0.5f, 10, WHITE);
        
        // Draw the mysterious head in the bowl (hide during some attack animations)
        bool showHead = true;
        if (isAttacking) {
            if (currentAttack == SPECIAL_NEUTRAL || currentAttack == SPECIAL_UP || 
                currentAttack == DOWN || currentAttack == SPECIAL_DOWN) {
                showHead = false;
            }
        }
        
        if (showHead) {
            DrawCircle(position.x, position.y - height * 0.4f, width * 0.15f, PINK);
            
            // Eyes for the head
            float headEyeOffset = 5;
            DrawCircle(position.x - headEyeOffset, position.y - height * 0.45f, width * 0.05f, BLACK);
            DrawCircle(position.x + headEyeOffset, position.y - height * 0.45f, width * 0.05f, BLACK);
        }
        
        // Special attack visualizations
        if (isAttacking) {
            switch (currentAttack) {
                case SPECIAL_NEUTRAL: // Water Cannon
                    {
                        float attackWidth = width * 3.0f * ((float)attackFrame / attackDuration);
                        float attackX = isFacingRight ? position.x + width/2 : position.x - width/2 - attackWidth;
                        Color waterColor = SKYBLUE;
                        waterColor.a = 180;
                        DrawRectangleRounded({attackX, position.y - height * 0.5f, attackWidth, height * 0.3f}, 0.5f, 10, waterColor);
                    }
                    break;
                case SPECIAL_SIDE: // Rolling Flush
                    {
                        Color spinColor = SKYBLUE;
                        spinColor.a = 150;
                        DrawCircleV(position, width * 0.8f, spinColor);
                    }
                    break;
                case SPECIAL_UP: // Geyser Recovery
                    {
                        Color geyserColor = SKYBLUE;
                        geyserColor.a = 200 - (attackFrame * 5);
                        DrawTriangle(
                            {position.x, position.y},
                            {position.x - width/2, position.y + height},
                            {position.x + width/2, position.y + height},
                            geyserColor
                        );
                    }
                    break;
                case SPECIAL_DOWN: // Swirl Counter
                    {
                        for (int i = 0; i < 8; i++) {
                            float angle = (i * 45.0f + attackFrame * 12.0f) * DEG2RAD;
                            float radius = width * (0.5f + (float)attackFrame / attackDuration);
                            Vector2 pos = {
                                position.x + (float)cos(angle) * radius,
                                position.y - height/2 + (float)sin(angle) * radius
                            };
                            Color swirlColor = SKYBLUE;
                            swirlColor.a = 150;
                            DrawCircleV(pos, 5.0f, swirlColor);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    } else if (name == "The Plunger") {
        // Draw plunger head (red rubber part)
        DrawCircle(position.x, position.y - height * 0.15f, width * 0.4f, RED);
        
        // Draw plunger stick
        DrawRectangle(position.x - width * 0.1f, position.y - height, width * 0.2f, height * 0.8f, BROWN);
    }
    
    // Draw eyes (for both characters)
    float eyeOffsetX = isFacingRight ? width * 0.2f : -width * 0.2f;
    DrawCircle(position.x + eyeOffsetX, position.y - height * 0.7f, width * 0.1f, WHITE);
    DrawCircle(position.x + eyeOffsetX, position.y - height * 0.7f, width * 0.05f, BLACK);
    
    // Draw damage percentage
    DrawText(TextFormat("%d%%", damage), (int)position.x - 20, (int)position.y - height - 30, 20, RED);
    
    // Draw cooldown indicators
    if (currentCooldown > 0) {
        DrawRectangle((int)position.x - 25, (int)position.y - height - 50, 50 * (float)currentCooldown / specialCooldown, 5, BLUE);
    }
    if (currentSideCooldown > 0) {
        DrawRectangle((int)position.x - 25, (int)position.y - height - 57, 50 * (float)currentSideCooldown / specialSideCooldown, 5, GREEN);
    }
    if (currentUpCooldown > 0) {
        DrawRectangle((int)position.x - 25, (int)position.y - height - 64, 50 * (float)currentUpCooldown / specialUpCooldown, 5, PURPLE);
    }
    if (currentDownCooldown > 0) {
        DrawRectangle((int)position.x - 25, (int)position.y - height - 71, 50 * (float)currentDownCooldown / specialDownCooldown, 5, YELLOW);
    }
    
    // Draw attacks
    for (auto& attack : attacks) {
        attack.draw();
    }
}

void Character::drawDeathAnimation() {
    // Calculate the alpha (transparency) based on death animation progress
    float alpha = 255.0f * (1.0f - (float)deathFrame / deathDuration);
    Color fadeColor = color;
    fadeColor.a = (unsigned char)alpha;
    
    // Draw character with rotation and scaling
    Rectangle charRect = {
        deathPosition.x - width/2 * deathScale,
        deathPosition.y - height * deathScale,
        width * deathScale,
        height * deathScale
    };
    
    // Draw using rotation
    Vector2 origin = {width/2 * deathScale, height * deathScale / 2};
    DrawRectanglePro(
        {charRect.x + origin.x, charRect.y + origin.y, charRect.width, charRect.height},
        origin,
        deathRotation,
        fadeColor
    );
    
    // Draw an explosion/blast effect
    if (deathFrame < 20) {
        float blastSize = 30.0f + deathFrame * 3.0f;
        Color blastColor = WHITE;
        blastColor.a = 255 - deathFrame * 12;
        DrawCircleV(deathPosition, blastSize, blastColor);
    }
}

void Character::jump() {
    if (!isJumping && !isAttacking && !isDying) {
        velocity.y = JUMP_FORCE;
        isJumping = true;
    }
}

void Character::moveLeft() {
    if ((!isAttacking || currentAttack == SPECIAL_SIDE) && !isDying) { // Allow movement during side special
        velocity.x = -speed;
        isFacingRight = false;
    }
}

void Character::moveRight() {
    if ((!isAttacking || currentAttack == SPECIAL_SIDE) && !isDying) { // Allow movement during side special
        velocity.x = speed;
        isFacingRight = true;
    }
}

// Standard attacks
void Character::neutralAttack() {
    if (!isAttacking && canAttack && !isDying) {
        isAttacking = true;
        canAttack = false;
        currentAttack = NEUTRAL;
        attackDuration = 15;
        attackFrame = 0;
        
        // Bowl Slap - quick hit with toilet lid
        float attackWidth = width * 1.2f;
        float attackX = isFacingRight ? position.x + width*0.3f : position.x - width*0.3f - attackWidth;
        
        Rectangle attackRect = {
            attackX, 
            position.y - height * 0.8f, 
            attackWidth, 
            height * 0.4f
        };
        
        // Only add the attack if we're not already attacking
        if (attacks.empty() || attacks.back().currentFrame > 2) {
            attacks.push_back(AttackBox(attackRect, 5, isFacingRight ? 5.0f : -5.0f, -2.0f, 10));
        }
    }
}

void Character::sideAttack() {
    if (!isAttacking && canAttack && !isDying) {
        isAttacking = true;
        canAttack = false;
        currentAttack = SIDE;
        attackDuration = 20;
        attackFrame = 0;
        
        // Flush Rush - charging forward attack
        float attackWidth = width * 1.5f;
        float attackX = isFacingRight ? position.x + width*0.4f : position.x - width*0.4f - attackWidth;
        
        Rectangle attackRect = {
            attackX, 
            position.y - height * 0.7f, 
            attackWidth, 
            height * 0.6f
        };
        
        // Only add if not already attacking
        if (attacks.empty() || attacks.back().currentFrame > 2) {
            attacks.push_back(AttackBox(attackRect, 8, isFacingRight ? 7.0f : -7.0f, -1.0f, 15));
        }
        
        // Add forward momentum
        velocity.x += isFacingRight ? 8.0f : -8.0f;
    }
}

void Character::upAttack() {
    if (!isAttacking && canAttack && !isDying) {
        isAttacking = true;
        canAttack = false;
        currentAttack = UP;
        attackDuration = 18;
        attackFrame = 0;
        
        // Lid Flip - upward attack with toilet lid
        Rectangle attackRect = {
            position.x - width * 0.7f, 
            position.y - height * 1.5f, 
            width * 1.4f, 
            height * 0.7f
        };
        
        // Only add if not already attacking
        if (attacks.empty() || attacks.back().currentFrame > 2) {
            attacks.push_back(AttackBox(attackRect, 7, isFacingRight ? 3.0f : -3.0f, -8.0f, 12));
        }
    }
}

void Character::downAttack() {
    if (!isAttacking && canAttack && !isDying) {
        isAttacking = true;
        canAttack = false;
        currentAttack = DOWN;
        attackDuration = 25;
        attackFrame = 0;
        
        // Splash Attack - creates damaging puddle below
        Rectangle attackRect = {
            position.x - width, 
            position.y, 
            width * 2.0f, 
            height * 0.5f
        };
        
        // Only add if not already attacking
        if (attacks.empty() || attacks.back().currentFrame > 2) {
            attacks.push_back(AttackBox(attackRect, 6, isFacingRight ? 2.0f : -2.0f, 6.0f, 20));
        }
    }
}

// Special attacks
void Character::specialNeutralAttack() {
    if (!isAttacking && canAttack && currentCooldown <= 0 && !isDying) {
        isAttacking = true;
        isSpecialAttack = true;
        canAttack = false;
        currentAttack = SPECIAL_NEUTRAL;
        attackDuration = 30;
        attackFrame = 0;
        currentCooldown = specialCooldown;
        
        // Water Cannon - shoots water that pushes enemies
        float attackWidth = width * 3.0f;
        float attackX = isFacingRight ? position.x + width/2 : position.x - width/2 - attackWidth;
        
        Rectangle attackRect = {
            attackX, 
            position.y - height * 0.6f, 
            attackWidth, 
            height * 0.3f
        };
        
        // Only add if not already attacking
        if (attacks.empty() || attacks.back().currentFrame > 2) {
            attacks.push_back(AttackBox(attackRect, 12, isFacingRight ? 12.0f : -12.0f, -3.0f, 25));
        }
    }
}

void Character::specialSideAttack() {
    if (!isAttacking && canAttack && currentSideCooldown <= 0 && !isDying) {
        isAttacking = true;
        isSpecialAttack = true;
        canAttack = false;
        currentAttack = SPECIAL_SIDE;
        attackDuration = 40;
        attackFrame = 0;
        currentSideCooldown = specialSideCooldown;
        
        // Rolling Flush - rolls forward while spinning water
        velocity.x = isFacingRight ? speed * 2.0f : -speed * 2.0f;
        
        // Create a circular attack area around the character
        Rectangle attackRect = {
            position.x - width, 
            position.y - height, 
            width * 2.0f, 
            height * 1.2f
        };
        
        // Only add if not already attacking
        if (attacks.empty() || attacks.back().currentFrame > 2) {
            attacks.push_back(AttackBox(attackRect, 10, isFacingRight ? 8.0f : -8.0f, -4.0f, 35));
        }
    }
}

void Character::specialUpAttack() {
    if (!isAttacking && currentUpCooldown <= 0 && !isDying) {
        isAttacking = true;
        isSpecialAttack = true;
        canAttack = false;
        currentAttack = SPECIAL_UP;
        attackDuration = 45;
        attackFrame = 0;
        isRecovering = true;
        currentUpCooldown = specialUpCooldown;
        
        // Geyser Recovery - water jet recovery move
        velocity.y = JUMP_FORCE * 1.5f;
        
        // Create attack hitbox below character
        Rectangle attackRect = {
            position.x - width/2, 
            position.y, 
            width, 
            height
        };
        
        // Only add if not already attacking
        if (attacks.empty() || attacks.back().currentFrame > 2) {
            attacks.push_back(AttackBox(attackRect, 8, 0, -10.0f, 20));
        }
    }
}

void Character::specialDownAttack() {
    if (!isAttacking && canAttack && currentDownCooldown <= 0 && !isDying) {
        isAttacking = true;
        isSpecialAttack = true;
        canAttack = false;
        currentAttack = SPECIAL_DOWN;
        attackDuration = 35;
        attackFrame = 0;
        currentDownCooldown = specialDownCooldown;
        
        // Swirl Counter - counter move with swirling water
        // Create circular attack area around character
        Rectangle attackRect = {
            position.x - width * 1.5f, 
            position.y - height * 1.5f, 
            width * 3.0f, 
            height * 2.0f
        };
        
        // Only add if not already attacking
        if (attacks.empty() || attacks.back().currentFrame > 2) {
            attacks.push_back(AttackBox(attackRect, 13, isFacingRight ? 9.0f : -9.0f, -7.0f, 30));
        }
    }
}

bool Character::checkHit(Character& other) {
    // Don't check hits if either character is dying
    if (isDying || other.isDying) return false;
    
    Rectangle otherRect = other.getRect();
    
    for (auto& attack : attacks) {
        if (CheckCollisionRecs(attack.rect, otherRect)) {
            // Apply damage
            other.damage += attack.damage;
            
            // Apply knockback (scaled by damage)
            float knockbackMultiplier = 1.0f + other.damage * KNOCKBACK_SCALING;
            other.velocity.x = attack.knockbackX * knockbackMultiplier;
            other.velocity.y = attack.knockbackY * knockbackMultiplier;
            
            // If damage is very high, potentially cause instant death (blast off)
            if (other.damage > 150 && GetRandomValue(0, 100) < 25) {
                other.startDeathAnimation();
            }
            
            return true;
        }
    }
    
    return false;
}