#include "../include/character/CharacterVisuals.h"
#include "../include/character/Character.h"
#include "../include/GameConfig.h"
#include <cmath>

// Linear interpolation helper function
inline float Lerp(float start, float end, float amount) {
    return start + amount * (end - start);
}

CharacterVisuals::CharacterVisuals(Character* character, CharacterStyle characterStyle, Color primary, Color secondary)
    : owner(character), facingLeft(false), visualScale(1.0f), style(characterStyle),
      mainColor(primary), secondaryColor(secondary), useShaders(false)
{
    // Set effect color based on character style
    switch (style) {
        case STYLE_BRAWLER:
            effectColor = RED;
            break;
        case STYLE_SPEEDY:
            effectColor = BLUE;
            break;
        case STYLE_HEAVY:
            effectColor = ORANGE;
            break;
        case STYLE_SWORD:
            effectColor = GREEN;
            break;
        case STYLE_CUSTOM:
            effectColor = PURPLE;
            break;
    }

    // Load appropriate spritesheet based on style
    loadSpriteSheet();

    // Setup animations
    setupAnimations();

    // Set default animation
    setAnimation("idle");
}

CharacterVisuals::~CharacterVisuals() {
    // Unload textures
    UnloadTexture(spriteSheet);

    // Unload shaders
    if (useShaders) {
        UnloadShader(outlineShader);
        UnloadShader(smashChargeShader);
    }
}

// Initialize the visuals system
void CharacterVisuals::InitShaders() {
    // Load common shaders here if your system supports them
    // If not, the useShaders flag will remain false
}

// Load appropriate sprite sheet based on style
void CharacterVisuals::loadSpriteSheet() {
    // In a real implementation, load actual spritesheets
    // For now, we'll simulate with colored rectangles

    // Create a temporary blank image
    Image tempImage = GenImageColor(512, 512, Color{0, 0, 0, 0});

    // Create a dummy spritesheet
    spriteSheet = LoadTextureFromImage(tempImage);
    UnloadImage(tempImage);
}

// Setup all animations for this character
void CharacterVisuals::setupAnimations() {
    // Common animations for all character types
    setupCommonAnimations();

    // Style-specific animations
    switch (style) {
        case STYLE_BRAWLER:
            setupBrawlerAnimations();
            break;
        case STYLE_SPEEDY:
            setupSpeedyAnimations();
            break;
        case STYLE_HEAVY:
            setupHeavyAnimations();
            break;
        case STYLE_SWORD:
            setupSwordAnimations();
            break;
        case STYLE_CUSTOM:
            setupCustomAnimations();
            break;
    }
}

// Setup animations common to all characters
void CharacterVisuals::setupCommonAnimations() {
    // Idle animation
    Animation idle;
    idle.loops = true;
    for (int i = 0; i < 4; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 0, 64, 64};
        frame.duration = 0.15f;
        frame.offset = {0, 0};
        idle.frames.push_back(frame);
        idle.totalDuration += frame.duration;
    }
    animations["idle"] = idle;

    // Running animation
    Animation running;
    running.loops = true;
    for (int i = 0; i < 6; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 64, 64, 64};
        frame.duration = 0.1f;
        frame.offset = {0, 0};
        running.frames.push_back(frame);
        running.totalDuration += frame.duration;
    }
    animations["running"] = running;

    // Jumping animation
    Animation jumping;
    jumping.loops = false;
    for (int i = 0; i < 4; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 128, 64, 64};
        frame.duration = 0.1f;
        frame.offset = {0, 0};
        jumping.frames.push_back(frame);
        jumping.totalDuration += frame.duration;
    }
    animations["jumping"] = jumping;

    // Falling animation
    Animation falling;
    falling.loops = true;
    for (int i = 0; i < 2; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 192, 64, 64};
        frame.duration = 0.2f;
        frame.offset = {0, 0};
        falling.frames.push_back(frame);
        falling.totalDuration += frame.duration;
    }
    animations["falling"] = falling;

    // Placeholder animations for other states
    Animation placeholder;
    placeholder.loops = false;
    AnimationFrame frame;
    frame.sourceRect = {0, 0, 64, 64};
    frame.duration = 0.1f;
    frame.offset = {0, 0};
    placeholder.frames.push_back(frame);
    placeholder.totalDuration = frame.duration;

    // Add placeholder animations for common states
    animations["shield"] = placeholder;
    animations["hitstun"] = placeholder;
    animations["dying"] = placeholder;
    animations["charge"] = placeholder;
    animations["spotdodge"] = placeholder;
    animations["forwarddodge"] = placeholder;
    animations["backdodge"] = placeholder;
    animations["grab"] = placeholder;

    // Add placeholder animations for attacks
    animations["jab"] = placeholder;
    animations["ftilt"] = placeholder;
    animations["utilt"] = placeholder;
    animations["dtilt"] = placeholder;
    animations["fsmash"] = placeholder;
    animations["usmash"] = placeholder;
    animations["dsmash"] = placeholder;
    animations["nair"] = placeholder;
    animations["fair"] = placeholder;
    animations["bair"] = placeholder;
    animations["uair"] = placeholder;
    animations["dair"] = placeholder;
    animations["nspecial"] = placeholder;
    animations["sspecial"] = placeholder;
    animations["uspecial"] = placeholder;
    animations["dspecial"] = placeholder;
}

// Style-specific animation setups
void CharacterVisuals::setupBrawlerAnimations() {
    // In a real implementation, these would load style-specific animation frames
    // For now, we'll just ensure all animations exist

    // Example: Jab with active hitbox
    Animation jab;
    jab.loops = false;
    for (int i = 0; i < 3; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 256, 64, 64};
        frame.duration = 0.05f;
        frame.offset = {0, 0};

        // Second frame has active hit
        if (i == 1) {
            frame.isHitActive = true;
            frame.hitboxOffset = {32, 0};
            frame.hitboxSize = {30, 20};
        } else {
            frame.isHitActive = false;
        }

        jab.frames.push_back(frame);
        jab.totalDuration += frame.duration;
    }
    animations["jab"] = jab;
}

void CharacterVisuals::setupSpeedyAnimations() {
    // Faster animations for speedy character
    // Placeholder implementation
}

void CharacterVisuals::setupHeavyAnimations() {
    // Slower, stronger animations for heavy character
    // Placeholder implementation
}

void CharacterVisuals::setupSwordAnimations() {
    // Animations with longer range for sword character
    // Placeholder implementation
}

void CharacterVisuals::setupCustomAnimations() {
    // Custom character animations
    // Placeholder implementation
}

// Set current animation
void CharacterVisuals::setAnimation(const std::string& animName) {
    // Don't change if it's the same animation
    if (currentAnimation == animName) {
        return;
    }

    // Check if animation exists
    auto it = animations.find(animName);
    if (it != animations.end()) {
        currentAnimation = animName;
        animations[currentAnimation].reset();
    }
}

// Update visuals
void CharacterVisuals::update(float deltaTime, bool isFacingLeft) {
    facingLeft = isFacingLeft;

    // Update current animation
    animations[currentAnimation].update(deltaTime, facingLeft);

    // Update visual effects
    for (int i = 0; i < effects.size(); i++) {
        if (!effects[i].update(deltaTime)) {
            effects.erase(effects.begin() + i);
            i--;
        }
    }

    // Update movement trail
    updateMovementTrail(deltaTime);

    // Update particles
    updateParticles(deltaTime);
}

// Update movement trail
void CharacterVisuals::updateMovementTrail(float deltaTime) {
    // Fade existing trail points
    for (int i = 0; i < movementTrail.size(); i++) {
        movementTrail[i].alpha -= deltaTime * 2.0f;
        if (movementTrail[i].alpha <= 0) {
            movementTrail.erase(movementTrail.begin() + i);
            i--;
        }
    }
}

// Update particles
void CharacterVisuals::updateParticles(float deltaTime) {
    for (int i = 0; i < particles.size(); i++) {
        // Update position
        particles[i].position.x += particles[i].velocity.x * deltaTime;
        particles[i].position.y += particles[i].velocity.y * deltaTime;

        // Apply gravity to velocity
        particles[i].velocity.y += 200.0f * deltaTime;

        // Update life
        particles[i].life -= deltaTime;

        // Remove dead particles
        if (particles[i].life <= 0) {
            particles.erase(particles.begin() + i);
            i--;
        }
    }
}

// Add trail point
void CharacterVisuals::addTrailPoint(Vector2 position) {
    // Only add trail points during fast movements
    TrailPoint trail;
    trail.position = position;
    trail.alpha = 0.7f;
    movementTrail.push_back(trail);

    // Limit trail size
    if (movementTrail.size() > 10) {
        movementTrail.erase(movementTrail.begin());
    }
}

// Add hit effect
void CharacterVisuals::addHitEffect(Vector2 position, float size, Color color) {
    VisualEffect effect;
    effect.position = position;
    effect.lifeSpan = 0.3f;
    effect.currentLife = effect.lifeSpan;
    effect.scale = size;
    effect.rotation = GetRandomValue(0, 360);
    effect.color = color;
    effect.effectType = 0; // Hit spark
    effects.push_back(effect);

    // Also add particles
    addHitParticles(position, 10, color);
}

// Add dust effect
void CharacterVisuals::addDustEffect(Vector2 position) {
    VisualEffect effect;
    effect.position = position;
    effect.lifeSpan = 0.5f;
    effect.currentLife = effect.lifeSpan;
    effect.scale = GetRandomValue(10, 20) / 10.0f;
    effect.rotation = GetRandomValue(0, 360);
    effect.color = LIGHTGRAY;
    effect.effectType = 1; // Dust
    effects.push_back(effect);
}

// Add shield effect
void CharacterVisuals::addShieldEffect(Vector2 position, float size, Color color) {
    VisualEffect effect;
    effect.position = position;
    effect.lifeSpan = 0.1f; // Short-lived
    effect.currentLife = effect.lifeSpan;
    effect.scale = size;
    effect.rotation = 0;
    effect.color = color;
    effect.effectType = 2; // Shield
    effects.push_back(effect);
}

// Add hit particles
void CharacterVisuals::addHitParticles(Vector2 position, int count, Color color) {
    for (int i = 0; i < count; i++) {
        Particle particle;
        particle.position = position;

        // Random velocity in all directions
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(50, 200);
        particle.velocity.x = cos(angle) * speed;
        particle.velocity.y = sin(angle) * speed;

        // Random size and life
        particle.size = GetRandomValue(2, 5);
        particle.life = GetRandomValue(5, 15) / 10.0f;

        // Color with slight variation
        particle.color = color;
        particle.color.r = std::min(255, particle.color.r + GetRandomValue(-20, 20));
        particle.color.g = std::min(255, particle.color.g + GetRandomValue(-20, 20));
        particle.color.b = std::min(255, particle.color.b + GetRandomValue(-20, 20));

        particles.push_back(particle);
    }
}

// Map attack type to animation
void CharacterVisuals::updateAnimationFromAttack(AttackType attackType) {
    switch (attackType) {
        case AttackType::JAB:
            setAnimation("jab");
            break;
        case AttackType::FORWARD_TILT:
            setAnimation("ftilt");
            break;
        case AttackType::UP_TILT:
            setAnimation("utilt");
            break;
        case AttackType::DOWN_TILT:
            setAnimation("dtilt");
            break;
        case AttackType::DASH_ATTACK:
            setAnimation("dash");
            break;
        case AttackType::FORWARD_SMASH:
            setAnimation("fsmash");
            break;
        case AttackType::UP_SMASH:
            setAnimation("usmash");
            break;
        case AttackType::DOWN_SMASH:
            setAnimation("dsmash");
            break;
        case AttackType::NEUTRAL_AIR:
            setAnimation("nair");
            break;
        case AttackType::FORWARD_AIR:
            setAnimation("fair");
            break;
        case AttackType::BACK_AIR:
            setAnimation("bair");
            break;
        case AttackType::UP_AIR:
            setAnimation("uair");
            break;
        case AttackType::DOWN_AIR:
            setAnimation("dair");
            break;
        case AttackType::NEUTRAL_SPECIAL:
            setAnimation("nspecial");
            break;
        case AttackType::SIDE_SPECIAL:
            setAnimation("sspecial");
            break;
        case AttackType::UP_SPECIAL:
            setAnimation("uspecial");
            break;
        case AttackType::DOWN_SPECIAL:
            setAnimation("dspecial");
            break;
        case AttackType::GRAB:
            setAnimation("grab");
            break;
        case AttackType::PUMMEL:
            setAnimation("pummel");
            break;
        case AttackType::FORWARD_THROW:
            setAnimation("fthrow");
            break;
        case AttackType::BACK_THROW:
            setAnimation("bthrow");
            break;
        case AttackType::UP_THROW:
            setAnimation("uthrow");
            break;
        case AttackType::DOWN_THROW:
            setAnimation("dthrow");
            break;
        default:
            break;
    }
}

// Map character state to animation
void CharacterVisuals::updateAnimationFromState(CharacterState state, bool isAttacking, bool isGrabbing) {
    if (isAttacking) {
        // Attack animations should already be set by updateAnimationFromAttack
        return;
    }

    if (isGrabbing) {
        setAnimation("grab");
        return;
    }

    // Map state to animation
    switch (state) {
        case CharacterState::IDLE:
            setAnimation("idle");
            break;
        case CharacterState::RUNNING:
            setAnimation("running");
            break;
        case CharacterState::JUMPING:
            setAnimation("jumping");
            break;
        case CharacterState::FALLING:
            setAnimation("falling");
            break;
        case CharacterState::SHIELDING:
            setAnimation("shield");
            break;
        case CharacterState::DODGING:
            setAnimation("spotdodge");
            break;
        case CharacterState::HITSTUN:
            setAnimation("hitstun");
            break;
        case CharacterState::DYING:
            setAnimation("dying");
            break;
        default:
            break;
    }
}

// Draw character
void CharacterVisuals::draw(Vector2 position, float width, float height, float damage) {
    // Draw movement trail if moving fast
    drawMovementTrail(position);

    // Draw particles behind character
    drawParticles();

    // Calculate damage glow based on damage percentage
    float damageGlow = damage / 150.0f; // Max glow at 150% damage
    damageGlow = damageGlow > 1.0f ? 1.0f : damageGlow;

    // Get current animation frame
    Animation& anim = animations[currentAnimation];
    AnimationFrame& frame = anim.getCurrentFrame();

    // Calculate draw position with offset
    Vector2 drawPos = {
        position.x + (facingLeft ? -frame.offset.x : frame.offset.x),
        position.y + frame.offset.y
    };

    // For now, draw a colored rectangle since we don't have actual sprites
    // In the actual implementation, you would draw the sprite instead

    // Draw character shadow
    DrawEllipse(
        position.x,
        position.y + height/2 + 5,
        width/2,
        10,
        Color{0, 0, 0, 100}
    );

    // Draw character body
    Rectangle bodyRect = {
        drawPos.x - width/2,
        drawPos.y - height,
        width,
        height
    };

    // Mix color with damage glow
    Color drawColor = mainColor;
    drawColor.r = Lerp(mainColor.r, 255, damageGlow);
    drawColor.g = Lerp(mainColor.g, 100, damageGlow);
    drawColor.b = Lerp(mainColor.b, 100, damageGlow);

    // Draw main body
    DrawRectangleRounded(bodyRect, 0.3f, 8, drawColor);

    // Draw head (slightly larger for cartoon look)
    Rectangle headRect = {
        drawPos.x - width/2 * 0.8f,
        drawPos.y - height - width/2 * 0.8f,
        width * 0.8f,
        width * 0.8f
    };
    DrawRectangleRounded(headRect, 0.5f, 8, drawColor);

    // Draw eyes
    float eyeSize = width * 0.15f;
    float eyeX = drawPos.x - (facingLeft ? -eyeSize : eyeSize);
    Rectangle eyeRect = {
        eyeX - eyeSize/2,
        drawPos.y - height - width/2 * 0.8f + width * 0.25f,
        eyeSize,
        eyeSize
    };
    DrawRectangleRounded(eyeRect, 0.8f, 8, BLACK);

    // Draw secondary details based on character style
    switch (style) {
        case STYLE_BRAWLER: {
            // Draw brawler details (e.g., cap, gloves)
            Rectangle capRect = {
                drawPos.x - width/2 * 0.8f,
                drawPos.y - height - width/2 * 0.8f - 5,
                width * 0.8f,
                10
            };
            DrawRectangleRounded(capRect, 0.3f, 8, secondaryColor);

            // Draw hands/gloves when attacking
            if (currentAnimation == "jab" ||
                currentAnimation == "fsmash" ||
                currentAnimation == "usmash" ||
                currentAnimation == "dsmash") {
                float handSize = width * 0.3f;
                float handX = drawPos.x + (facingLeft ? -width/2 - handSize : width/2);
                Rectangle handRect = {
                    handX,
                    drawPos.y - height * 0.5f,
                    handSize,
                    handSize
                };
                DrawRectangleRounded(handRect, 0.8f, 8, secondaryColor);
            }
            break;
        }
        case STYLE_SPEEDY: {
            // Draw speedy details (e.g., ears, tail)
            // Ears
            Rectangle leftEarRect = {
                drawPos.x - width/2 * 0.6f - 5,
                drawPos.y - height - width/2 * 0.8f - 15,
                10,
                20
            };
            DrawRectangleRounded(leftEarRect, 0.5f, 8, secondaryColor);

            Rectangle rightEarRect = {
                drawPos.x + width/2 * 0.6f - 5,
                drawPos.y - height - width/2 * 0.8f - 15,
                10,
                20
            };
            DrawRectangleRounded(rightEarRect, 0.5f, 8, secondaryColor);

            // Tail
            Rectangle tailRect = {
                drawPos.x - (facingLeft ? width/2 : -width/2),
                drawPos.y - height * 0.3f,
                width * 0.5f,
                10
            };
            DrawRectangleRounded(tailRect, 0.8f, 8, secondaryColor);
            break;
        }
        case STYLE_HEAVY: {
            // Draw heavy details (e.g., shell, spikes)
            Rectangle shellRect = {
                drawPos.x - width/2 * 0.9f,
                drawPos.y - height * 0.9f,
                width * 0.9f,
                height * 0.5f
            };
            DrawRectangleRounded(shellRect, 0.3f, 8, secondaryColor);

            // Spikes
            for (int i = 0; i < 3; i++) {
                Rectangle spikeRect = {
                    drawPos.x - width/2 * 0.7f + i * (width * 0.3f),
                    drawPos.y - height * 0.9f - 10,
                    10,
                    10
                };
                DrawRectangleRounded(spikeRect, 0.5f, 8, Color{120, 60, 20, 255});
            }
            break;
        }
        case STYLE_SWORD: {
            // Draw sword details (e.g., hat, sword)
Rectangle hatRect = {
                drawPos.x - width/2 * 0.9f,
                drawPos.y - height - width/2 * 0.8f - 5,
                width * 0.9f,
                15
            };
            DrawRectangleRounded(hatRect, 0.3f, 8, secondaryColor);

            // Draw sword when attacking
            if (currentAnimation.find("attack") != std::string::npos ||
                currentAnimation.find("air") != std::string::npos ||
                currentAnimation.find("smash") != std::string::npos ||
                currentAnimation.find("tilt") != std::string::npos ||
                currentAnimation.find("special") != std::string::npos) {

                float swordLength = width * 1.2f;
                float swordWidth = 8;
                float swordX = drawPos.x + (facingLeft ? -width/2 - swordLength : width/2);

                // Sword blade
                Rectangle swordRect = {
                    swordX,
                    drawPos.y - height * 0.6f,
                    swordLength,
                    swordWidth
                };
                DrawRectangleRounded(swordRect, 0.1f, 8, LIGHTGRAY);

                // Sword handle
                Rectangle handleRect = {
                    drawPos.x + (facingLeft ? -width/2 - 15 : width/2),
                    drawPos.y - height * 0.6f - 10,
                    15,
                    30
                };
                DrawRectangleRounded(handleRect, 0.5f, 8, DARKBROWN);
            }
            break;
        }
        case STYLE_CUSTOM:
            // Custom style details
            break;
    }

    // Draw active hit effects
    for (const auto& effect : effects) {
        switch (effect.effectType) {
            case 0: { // Hit spark
                float alpha = effect.currentLife / effect.lifeSpan;
                Color sparkColor = effect.color;
                sparkColor.a = alpha * 255;

                // Draw hit spark as star shape
                float outerRadius = effect.scale * 20.0f;
                float innerRadius = outerRadius * 0.5f;
                int points = 8;

                for (int i = 0; i < points * 2; i++) {
                    float radius = i % 2 == 0 ? outerRadius : innerRadius;
                    float angle = effect.rotation + i * 360.0f/(points * 2) * DEG2RAD;
                    Vector2 start = {
                        effect.position.x + cos(angle) * radius,
                        effect.position.y + sin(angle) * radius
                    };

                    radius = (i+1) % 2 == 0 ? outerRadius : innerRadius;
                    angle = effect.rotation + (i+1) * 360.0f/(points * 2) * DEG2RAD;
                    Vector2 end = {
                        effect.position.x + cos(angle) * radius,
                        effect.position.y + sin(angle) * radius
                    };

                    DrawLineEx(start, end, 3.0f, sparkColor);
                }
                break;
            }
            case 1: { // Dust
                float alpha = effect.currentLife / effect.lifeSpan;
                Color dustColor = effect.color;
                dustColor.a = alpha * 200;

                DrawCircle(
                    effect.position.x,
                    effect.position.y,
                    effect.scale * 10.0f * (1.0f - alpha * 0.5f),
                    dustColor
                );
                break;
            }
            case 2: { // Shield
                float alpha = effect.currentLife / effect.lifeSpan;
                Color shieldColor = effect.color;
                shieldColor.a = alpha * 150;

                DrawCircle(
                    effect.position.x,
                    effect.position.y,
                    effect.scale,
                    shieldColor
                );
                break;
            }
            case 3: { // Smash charge
                float alpha = effect.currentLife / effect.lifeSpan;
                Color chargeColor = effect.color;
                chargeColor.a = alpha * 200;

                // Draw swirling energy effect
                float time = GetTime() * 5.0f;
                for (int i = 0; i < 8; i++) {
                    float angle = time + i * 45.0f * DEG2RAD;
                    float distance = effect.scale * (0.5f + sin(time * 2.0f) * 0.2f);
                    Vector2 pos = {
                        effect.position.x + cos(angle) * distance,
                        effect.position.y + sin(angle) * distance
                    };

                    DrawCircle(pos.x, pos.y, 5.0f, chargeColor);
                }
                break;
            }
        }
    }
}

// Draw movement trail
void CharacterVisuals::drawMovementTrail(Vector2 position) {
    for (const auto& trail : movementTrail) {
        Color trailColor = mainColor;
        trailColor.a = trail.alpha * 150;

        DrawCircle(
            trail.position.x,
            trail.position.y,
            10.0f * trail.alpha,
            trailColor
        );
    }
}

// Draw particles
void CharacterVisuals::drawParticles() {
    for (const auto& particle : particles) {
        Color particleColor = particle.color;
        particleColor.a = (particle.life / 1.5f) * 255;

        DrawCircle(
            particle.position.x,
            particle.position.y,
            particle.size,
            particleColor
        );
    }
}

// Get current animation name
const std::string& CharacterVisuals::getCurrentAnimation() const {
    return currentAnimation;
}

// Add smash charge effect
void CharacterVisuals::addSmashChargeEffect(Vector2 position, float power) {
    VisualEffect effect;
    effect.position = position;
    effect.lifeSpan = 0.1f; // Keep refreshing this
    effect.currentLife = effect.lifeSpan;
    effect.scale = power * 30.0f;
    effect.rotation = 0;
    effect.color = effectColor;
    effect.effectType = 3; // Smash charge
    effects.push_back(effect);
}

// Draw death animation
void CharacterVisuals::drawDeathAnimation(Vector2 position, float width, float height, float rotation, float scale, float damage) {
    // Calculate damage glow based on damage percentage
    float damageGlow = damage / 150.0f; // Max glow at 150% damage
    damageGlow = damageGlow > 1.0f ? 1.0f : damageGlow;

    // Mix color with damage glow
    Color drawColor = mainColor;
    drawColor.r = Lerp(mainColor.r, 255, damageGlow);
    drawColor.g = Lerp(mainColor.g, 100, damageGlow);
    drawColor.b = Lerp(mainColor.b, 100, damageGlow);

    // Draw spinning, shrinking character
    Rectangle destRect = {
        position.x,
        position.y,
        width * scale,
        height * scale
    };

    // Center the rectangle for rotation
    destRect.x -= destRect.width / 2;
    destRect.y -= destRect.height / 2;

    // Draw rotated rectangle
    DrawRectanglePro(
        destRect,
        {destRect.width / 2, destRect.height / 2},
        rotation,
        drawColor
    );

    // Add dynamic star bursts during death animation
    if (scale < 0.8f && scale > 0.2f && GetRandomValue(0, 10) < 3) {
        float starAngle = static_cast<float>(GetRandomValue(0, 360));
        float starDist = static_cast<float>(GetRandomValue(10, 30));
        Vector2 starPos = {
            position.x + cosf(starAngle * DEG2RAD) * starDist,
            position.y + sinf(starAngle * DEG2RAD) * starDist
        };

        DrawCircleV(starPos, 5.0f * scale, WHITE);
    }
}

// Draw explosion effect
void CharacterVisuals::drawExplosionEffect(Vector2 position, int frame, int totalFrames) {
    // Calculate explosion progress (0.0 to 1.0)
    float progress = static_cast<float>(frame) / totalFrames;

    // Draw shockwave
    float shockwaveRadius = frame * 8.0f;
    float alpha = 255 * (1.0f - progress);
    Color shockwaveColor = {255, 200, 50, static_cast<unsigned char>(alpha)};

    DrawCircleLines(position.x, position.y, shockwaveRadius, shockwaveColor);
    DrawCircleLines(position.x, position.y, shockwaveRadius * 0.7f, shockwaveColor);

    // Draw flash effect in early frames
    if (frame < 10) {
        Color flashColor = {255, 255, 255,
                           static_cast<unsigned char>(255 * (1.0f - frame / 10.0f))};
        DrawRectangle(0, 0, GameConfig::SCREEN_WIDTH, GameConfig::SCREEN_HEIGHT, flashColor);
    }

    // Draw multiple circles for explosion effect
    if (frame < totalFrames * 0.5f) {
        float explosionSize = 50.0f * (frame / static_cast<float>(totalFrames * 0.3f));

        // Inner explosion
        Color innerColor = {255, 128, 0, static_cast<unsigned char>((1.0f - progress) * 255)};
        DrawCircleV(position, explosionSize, innerColor);

        // Outer explosion
        Color outerColor = {255, 200, 0, static_cast<unsigned char>((1.0f - progress) * 150)};
        DrawCircleV(position, explosionSize * 1.5f, outerColor);
    }
}