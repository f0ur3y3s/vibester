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

    // Jab attack animation
    Animation jab;
    jab.loops = false;
    for (int i = 0; i < 3; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 256, 64, 64};
        frame.duration = 0.05f;
        frame.offset = {0, 0};

        // Mark the middle frame as active hit
        if (i == 1) {
            frame.isHitActive = true;
            frame.hitboxOffset = {32, 0};
            frame.hitboxSize = {30, 20};
        }

        jab.frames.push_back(frame);
        jab.totalDuration += frame.duration;
    }
    animations["jab"] = jab;

    // Forward tilt animation
    Animation ftilt;
    ftilt.loops = false;
    for (int i = 0; i < 4; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 320, 64, 64};
        frame.duration = 0.07f;
        frame.offset = {i == 2 ? 10.0f : 0, 0};

        // Mark hit active frames
        if (i == 2) {
            frame.isHitActive = true;
            frame.hitboxOffset = {35, 0};
            frame.hitboxSize = {40, 30};
        }

        ftilt.frames.push_back(frame);
        ftilt.totalDuration += frame.duration;
    }
    animations["ftilt"] = ftilt;

    // Neutral air animation
    Animation nair;
    nair.loops = false;
    for (int i = 0; i < 5; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 384, 64, 64};
        frame.duration = 0.06f;
        frame.offset = {0, 0};

        // Middle frames have active hit
        if (i >= 1 && i <= 3) {
            frame.isHitActive = true;
            frame.hitboxOffset = {0, 0};
            frame.hitboxSize = {60, 60}; // Circular hitbox
        }

        nair.frames.push_back(frame);
        nair.totalDuration += frame.duration;
    }
    animations["nair"] = nair;

    // Add all other required animations with basic placeholder frames
    setupPlaceholderAnimations();
}

void CharacterVisuals::setupPlaceholderAnimations() {
    // Create placeholder for other attack animations
    Animation placeholder;
    placeholder.loops = false;
    AnimationFrame frame;
    frame.sourceRect = {0, 0, 64, 64};
    frame.duration = 0.1f;
    frame.offset = {0, 0};
    placeholder.frames.push_back(frame);
    placeholder.totalDuration = frame.duration;

    // Add placeholder animations for all states we haven't manually defined
    const std::string animationNames[] = {
        "shield", "hitstun", "dying", "charge", "spotdodge", "forwarddodge", "backdodge", "grab",
        "utilt", "dtilt", "dash", "fsmash", "usmash", "dsmash", "fair", "bair", "uair", "dair",
        "nspecial", "sspecial", "uspecial", "dspecial", "pummel", "fthrow", "bthrow", "uthrow", "dthrow"
    };

    for (const auto& name : animationNames) {
        if (animations.find(name) == animations.end()) {
            animations[name] = placeholder;
        }
    }
}

// Style-specific animation setups
void CharacterVisuals::setupBrawlerAnimations() {
    // Enhance brawler jab animation
    if (animations.find("jab") != animations.end()) {
        animations["jab"].frames[1].hitboxOffset = {40, 0};
        animations["jab"].frames[1].hitboxSize = {35, 25};
    }

    // Enhance forward smash for brawler
    Animation fsmash;
    fsmash.loops = false;

    // Windup frames
    for (int i = 0; i < 2; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 448, 64, 64};
        frame.duration = 0.08f;
        frame.offset = {i == 1 ? -10.0f : 0, 0}; // Pull back before punch
        frame.isHitActive = false;
        fsmash.frames.push_back(frame);
        fsmash.totalDuration += frame.duration;
    }

    // Active hit frames
    for (int i = 2; i < 4; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 448, 64, 64};
        frame.duration = 0.06f;
        frame.offset = {20.0f, 0}; // Extend forward for punch
        frame.isHitActive = true;
        frame.hitboxOffset = {45, 0};
        frame.hitboxSize = {50, 40};
        fsmash.frames.push_back(frame);
        fsmash.totalDuration += frame.duration;
    }

    // End lag frames
    for (int i = 4; i < 5; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 448, 64, 64};
        frame.duration = 0.12f;
        frame.offset = {0, 0};
        frame.isHitActive = false;
        fsmash.frames.push_back(frame);
        fsmash.totalDuration += frame.duration;
    }

    animations["fsmash"] = fsmash;
}

void CharacterVisuals::setupSpeedyAnimations() {
    // Create unique speed character animations

    // Speed character's neutral air is a quick spin
    Animation nair;
    nair.loops = false;

    for (int i = 0; i < 6; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 384, 64, 64};
        frame.duration = 0.04f; // Faster animation
        frame.offset = {0, 0};

        // Active on frames 1-4
        if (i >= 1 && i <= 4) {
            frame.isHitActive = true;
            frame.hitboxOffset = {0, 0};
            frame.hitboxSize = {70, 40}; // Wide hitbox for spin
        }

        nair.frames.push_back(frame);
        nair.totalDuration += frame.duration;
    }

    animations["nair"] = nair;

    // Speedy up special is a quick recovery move
    Animation uspecial;
    uspecial.loops = false;

    for (int i = 0; i < 5; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 512, 64, 64};
        frame.duration = 0.05f;
        frame.offset = {0, i > 0 ? -15.0f : 0}; // Move upward during animation

        if (i >= 1 && i <= 3) {
            frame.isHitActive = true;
            frame.hitboxOffset = {0, -20};
            frame.hitboxSize = {40, 60};
        }

        uspecial.frames.push_back(frame);
        uspecial.totalDuration += frame.duration;
    }

    animations["uspecial"] = uspecial;
}

void CharacterVisuals::setupHeavyAnimations() {
    // Heavy character animations are slower but more powerful

    // Heavy down smash is a ground pound
    Animation dsmash;
    dsmash.loops = false;

    // Windup
    for (int i = 0; i < 2; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 576, 64, 64};
        frame.duration = 0.1f; // Slower windup
        frame.offset = {0, i == 1 ? -10.0f : 0}; // Jump up slightly
        frame.isHitActive = false;
        dsmash.frames.push_back(frame);
        dsmash.totalDuration += frame.duration;
    }

    // Impact
    for (int i = 2; i < 4; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 576, 64, 64};
        frame.duration = 0.08f;
        frame.offset = {0, 5.0f}; // Land with impact
        frame.isHitActive = true;
        frame.hitboxOffset = {0, 30};
        frame.hitboxSize = {80, 20}; // Wide, flat hitbox
        dsmash.frames.push_back(frame);
        dsmash.totalDuration += frame.duration;
    }

    // End lag
    AnimationFrame endFrame;
    endFrame.sourceRect = {4 * 64.0f, 576, 64, 64};
    endFrame.duration = 0.15f;
    endFrame.offset = {0, 0};
    endFrame.isHitActive = false;
    dsmash.frames.push_back(endFrame);
    dsmash.totalDuration += endFrame.duration;

    animations["dsmash"] = dsmash;
}

void CharacterVisuals::setupSwordAnimations() {
    // Sword character has long range but slower attacks

    // Forward air is a sword slash
    Animation fair;
    fair.loops = false;

    for (int i = 0; i < 5; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 640, 64, 64};
        frame.duration = 0.06f;
        frame.offset = {i == 2 ? 15.0f : 0, 0};

        if (i == 2 || i == 3) {
            frame.isHitActive = true;
            frame.hitboxOffset = {50, 0};
            frame.hitboxSize = {70, 30}; // Long sword hitbox
        }

        fair.frames.push_back(frame);
        fair.totalDuration += frame.duration;
    }

    animations["fair"] = fair;

    // Down tilt is a low sword sweep
    Animation dtilt;
    dtilt.loops = false;

    for (int i = 0; i < 4; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 704, 64, 64};
        frame.duration = 0.07f;
        frame.offset = {i == 2 ? 10.0f : 0, 0};

        if (i == 1 || i == 2) {
            frame.isHitActive = true;
            frame.hitboxOffset = {30, 30};
            frame.hitboxSize = {80, 20}; // Low-hitting sword sweep
        }

        dtilt.frames.push_back(frame);
        dtilt.totalDuration += frame.duration;
    }

    animations["dtilt"] = dtilt;
}

void CharacterVisuals::setupCustomAnimations() {
    // Custom character has unique properties mixing other styles

    // Neutral special is a charge-and-release projectile
    Animation nspecial;
    nspecial.loops = false;

    // Charge frames
    for (int i = 0; i < 3; i++) {
        AnimationFrame frame;
        frame.sourceRect = {i * 64.0f, 768, 64, 64};
        frame.duration = 0.1f;
        frame.offset = {0, 0};
        frame.isHitActive = false;
        nspecial.frames.push_back(frame);
        nspecial.totalDuration += frame.duration;
    }

    // Release frame
    AnimationFrame releaseFrame;
    releaseFrame.sourceRect = {3 * 64.0f, 768, 64, 64};
    releaseFrame.duration = 0.08f;
    releaseFrame.offset = {10.0f, 0};
    releaseFrame.isHitActive = true;
    releaseFrame.hitboxOffset = {40, 0};
    releaseFrame.hitboxSize = {30, 30}; // Projectile hitbox
    nspecial.frames.push_back(releaseFrame);
    nspecial.totalDuration += releaseFrame.duration;

    // End lag
    AnimationFrame endFrame;
    endFrame.sourceRect = {4 * 64.0f, 768, 64, 64};
    endFrame.duration = 0.12f;
    endFrame.offset = {0, 0};
    endFrame.isHitActive = false;
    nspecial.frames.push_back(endFrame);
    nspecial.totalDuration += endFrame.duration;

    animations["nspecial"] = nspecial;
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

    // Draw shadow underneath character
    DrawEllipse(
        position.x,
        position.y + height/2 + 5,
        width/2,
        10,
        Color{0, 0, 0, 100}
    );

    // Draw character based on style
    switch (style) {
        case STYLE_BRAWLER:
            drawBrawlerCharacter(drawPos, width, height, damageGlow);
            break;
        case STYLE_SPEEDY:
            drawSpeedyCharacter(drawPos, width, height, damageGlow);
            break;
        case STYLE_HEAVY:
            drawHeavyCharacter(drawPos, width, height, damageGlow);
            break;
        case STYLE_SWORD:
            drawSwordCharacter(drawPos, width, height, damageGlow);
            break;
        case STYLE_CUSTOM:
            drawCustomCharacter(drawPos, width, height, damageGlow);
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

    // Draw hitbox visualization for debug purposes
    if (frame.isHitActive) {
        Vector2 hitboxPos = {
            drawPos.x + (facingLeft ? -frame.hitboxOffset.x : frame.hitboxOffset.x),
            drawPos.y + frame.hitboxOffset.y
        };

        // Visual representation of active hitbox - semi-transparent
        Color hitboxColor = {255, 0, 0, 100};
        DrawRectangle(
            hitboxPos.x - frame.hitboxSize.x/2,
            hitboxPos.y - frame.hitboxSize.y/2,
            frame.hitboxSize.x,
            frame.hitboxSize.y,
            hitboxColor
        );
    }
}

// Draw brawler character
// Draw speedy character
void CharacterVisuals::drawSpeedyCharacter(Vector2 position, float width, float height, float damageGlow) {
    // Mix color with damage glow
    Color drawColor = mainColor;
    drawColor.r = Lerp(mainColor.r, 255, damageGlow);
    drawColor.g = Lerp(mainColor.g, 100, damageGlow);
    drawColor.b = Lerp(mainColor.b, 100, damageGlow);

    // Body - slimmer and more aerodynamic
    DrawRectangleRounded(
        {position.x - width/2 * 0.7f, position.y - height, width * 0.7f, height},
        0.5f, 8, drawColor
    );

    // Head - smaller and more pointed
    DrawRectangleRounded(
        {position.x - width/2 * 0.7f, position.y - height - width/2 * 0.7f, width * 0.7f, width * 0.7f},
        0.8f, 8, drawColor
    );

    // Pointed ears
    DrawRectangleRounded(
        {position.x - width/2 * 0.6f - 5, position.y - height - width/2 * 0.8f - 15, 10, 20},
        0.5f, 8, secondaryColor
    );

    DrawRectangleRounded(
        {position.x + width/2 * 0.6f - 5, position.y - height - width/2 * 0.8f - 15, 10, 20},
        0.5f, 8, secondaryColor
    );

    // Face - faster character has sharp eyes
    float eyeSize = width * 0.12f;
    float eyeX = position.x - (facingLeft ? -eyeSize*1.5f : eyeSize*1.5f);

    // Angled eyes for speedy look
    DrawRectangleRounded(
        {eyeX - eyeSize/2, position.y - height - width/2 * 0.8f + width * 0.25f, eyeSize * 1.5f, eyeSize * 0.7f},
        0.8f, 8, BLACK
    );

    // Smirk
    if (currentAnimation == "running" || currentAnimation.find("air") != std::string::npos) {
        // Confident smirk when running or in air
        DrawRectangleRounded(
            {position.x - width * 0.25f, position.y - height - width/2 * 0.8f + width * 0.5f, width * 0.4f, width * 0.08f},
            0.8f, 8, BLACK
        );
    } else {
        // Normal smile
        DrawRectangleRounded(
            {position.x - width * 0.3f, position.y - height - width/2 * 0.8f + width * 0.5f, width * 0.5f, width * 0.08f},
            0.8f, 8, BLACK
        );
    }

    // Tail for speedy character
    float tailWidth = width * 0.5f;
    float tailHeight = 10;
    float tailX = position.x - (facingLeft ? width/2 : -width/2);
    float tailY = position.y - height * 0.3f;

    DrawRectangleRounded(
        {tailX, tailY, tailWidth * (facingLeft ? -1 : 1), tailHeight},
        0.8f, 8, secondaryColor
    );

    // Speed boost when running/jumping
    if (currentAnimation == "running" || currentAnimation == "jumping") {
        // Speed lines
        for (int i = 0; i < 3; i++) {
            float lineLength = width * (0.5f + i * 0.2f);
            float lineY = position.y - height * (0.3f + i * 0.2f);

            DrawLineEx(
                {position.x - (facingLeft ? -width/4 : width/4), lineY},
                {position.x - (facingLeft ? -width/4 - lineLength : width/4 + lineLength), lineY},
                2.0f,
                secondaryColor
            );
        }
    }
}

// Draw heavy character
void CharacterVisuals::drawHeavyCharacter(Vector2 position, float width, float height, float damageGlow) {
    // Mix color with damage glow
    Color drawColor = mainColor;
    drawColor.r = Lerp(mainColor.r, 255, damageGlow);
    drawColor.g = Lerp(mainColor.g, 100, damageGlow);
    drawColor.b = Lerp(mainColor.b, 100, damageGlow);

    // Main body - larger and bulkier
    DrawRectangleRounded(
        {position.x - width/2 * 1.2f, position.y - height, width * 1.2f, height},
        0.2f, 8, drawColor
    );

    // Shell on back
    DrawRectangleRounded(
        {position.x - width/2 * 0.9f, position.y - height * 0.9f, width * 0.9f, height * 0.5f},
        0.3f, 8, secondaryColor
    );

    // Spikes on shell
    for (int i = 0; i < 3; i++) {
        Rectangle spikeRect = {
            position.x - width/2 * 0.7f + i * (width * 0.3f),
            position.y - height * 0.9f - 10,
            10,
            10
        };
        DrawRectangleRounded(spikeRect, 0.5f, 8, Color{120, 60, 20, 255});
    }

    // Head - smaller in proportion
    DrawRectangleRounded(
        {position.x - width/2 * 0.7f, position.y - height - width/2 * 0.7f, width * 0.7f, width * 0.7f},
        0.4f, 8, drawColor
    );

    // Face - angry looking
    float eyeSize = width * 0.14f;
    float eyeX = position.x - (facingLeft ? -eyeSize : eyeSize);

    // Angry eyebrows
    DrawLineEx(
        {eyeX - eyeSize, position.y - height - width/2 * 0.8f + width * 0.2f},
        {eyeX, position.y - height - width/2 * 0.8f + width * 0.15f},
        3.0f,
        BLACK
    );

    // Eyes
    DrawRectangleRounded(
        {eyeX - eyeSize/2, position.y - height - width/2 * 0.8f + width * 0.25f, eyeSize, eyeSize},
        0.8f, 8, BLACK
    );

    // Mouth - snarl
    if (currentAnimation.find("smash") != std::string::npos) {
        // Angry snarl during smash attacks
        DrawRectangleRounded(
            {position.x - width * 0.2f, position.y - height - width/2 * 0.8f + width * 0.5f, width * 0.4f, width * 0.15f},
            0.8f, 8, BLACK
        );
    } else {
        // Determined expression
        DrawRectangleRounded(
            {position.x - width * 0.3f, position.y - height - width/2 * 0.8f + width * 0.5f, width * 0.6f, width * 0.1f},
            0.1f, 8, BLACK
        );
    }

    // Heavy character has large fists
    if (currentAnimation.find("attack") != std::string::npos ||
        currentAnimation.find("smash") != std::string::npos ||
        currentAnimation == "jab") {

        float fistSize = width * 0.5f;
        float fistX = position.x + (facingLeft ? -width/2 - fistSize*0.7f : width/2);
        float fistY = position.y - height * 0.5f;

        // Draw large fist
        DrawRectangleRounded(
            {fistX, fistY, fistSize * (facingLeft ? -1 : 1), fistSize},
            0.5f, 8, drawColor
        );
    }
}

// Draw sword character
void CharacterVisuals::drawSwordCharacter(Vector2 position, float width, float height, float damageGlow) {
    // Mix color with damage glow
    Color drawColor = mainColor;
    drawColor.r = Lerp(mainColor.r, 255, damageGlow);
    drawColor.g = Lerp(mainColor.g, 100, damageGlow);
    drawColor.b = Lerp(mainColor.b, 100, damageGlow);

    // Hero-like body
    DrawRectangleRounded(
        {position.x - width/2 * 0.9f, position.y - height, width * 0.9f, height},
        0.3f, 8, drawColor
    );

    // Head with hero hat
    DrawRectangleRounded(
        {position.x - width/2 * 0.7f, position.y - height - width/2 * 0.7f, width * 0.7f, width * 0.7f},
        0.5f, 8, drawColor
    );

    // Hero hat/cap
    DrawRectangleRounded(
        {position.x - width/2 * 0.9f, position.y - height - width/2 * 0.8f - 5, width * 0.9f, 15},
        0.3f, 8, secondaryColor
    );

    // Face - determined hero look
    float eyeSize = width * 0.12f;
    float eyeX = position.x - (facingLeft ? -eyeSize*1.5f : eyeSize*1.5f);

    // Focused eyes
    DrawRectangleRounded(
        {eyeX - eyeSize/2, position.y - height - width/2 * 0.8f + width * 0.25f, eyeSize, eyeSize},
        0.8f, 8, BLACK
    );

    // Determined expression
    DrawRectangleRounded(
        {position.x - width * 0.25f, position.y - height - width/2 * 0.8f + width * 0.5f, width * 0.5f, width * 0.08f},
        0.5f, 8, BLACK
    );

    // Draw sword when attacking
    if (currentAnimation.find("attack") != std::string::npos ||
        currentAnimation.find("air") != std::string::npos ||
        currentAnimation.find("smash") != std::string::npos ||
        currentAnimation.find("tilt") != std::string::npos ||
        currentAnimation.find("special") != std::string::npos) {

        float swordLength = width * 1.5f;
        float swordWidth = 8;
        float swordX = position.x + (facingLeft ? -width/2 - swordLength : width/2);
        float swordY = position.y - height * 0.6f;

        // Sword handle
        DrawRectangleRounded(
            {position.x + (facingLeft ? -width/2 - 15 : width/2), swordY - 10, 15 * (facingLeft ? -1 : 1), 30},
            0.5f, 8, DARKBROWN
        );

        // Cross guard
        DrawRectangleRounded(
            {position.x + (facingLeft ? -width/2 - 25 : width/2 - 5), swordY - 5, 30 * (facingLeft ? -1 : 1), 10},
            0.3f, 8, GOLD
        );

        // Sword blade
        DrawRectangleRounded(
            {swordX, swordY, swordLength * (facingLeft ? -1 : 1), swordWidth},
            0.1f, 8, LIGHTGRAY
        );

        // Sword tip
        DrawTriangle(
            {swordX + (facingLeft ? 0 : swordLength), swordY + swordWidth/2},
            {swordX + (facingLeft ? -20 : swordLength + 20), swordY + swordWidth/2},
            {swordX + (facingLeft ? -10 : swordLength + 10), swordY - 5},
            LIGHTGRAY
        );

        // Sword glow during special attacks
        if (currentAnimation.find("special") != std::string::npos) {
            DrawRectangleRounded(
                {swordX, swordY, swordLength * (facingLeft ? -1 : 1), swordWidth},
                0.1f, 8, {220, 220, 255, 150}
            );
        }
    }
}

// Draw custom character
void CharacterVisuals::drawCustomCharacter(Vector2 position, float width, float height, float damageGlow) {
    // Mix color with damage glow
    Color drawColor = mainColor;
    drawColor.r = Lerp(mainColor.r, 255, damageGlow);
    drawColor.g = Lerp(mainColor.g, 100, damageGlow);
    drawColor.b = Lerp(mainColor.b, 100, damageGlow);

    // Unique mystic-looking body
    DrawRectangleRounded(
        {position.x - width/2 * 0.8f, position.y - height, width * 0.8f, height},
        0.4f, 8, drawColor
    );

    // Cloak/cape effect
    if (currentAnimation == "idle" || currentAnimation.find("special") != std::string::npos) {
        // Draw flowing cape
        DrawTriangle(
            {position.x - width/2 * 0.8f, position.y - height * 0.8f},
            {position.x + width/2 * 0.8f, position.y - height * 0.8f},
            {position.x, position.y - height * 0.2f},
            secondaryColor
        );
    }

    // Mystical head with glow
    DrawRectangleRounded(
        {position.x - width/2 * 0.7f, position.y - height - width/2 * 0.7f, width * 0.7f, width * 0.7f},
        0.8f, 8, drawColor
    );

    // Mystic headband/crown
    DrawRectangleRounded(
        {position.x - width/2 * 0.8f, position.y - height - width/2 * 0.5f - 5, width * 0.8f, 10},
        0.5f, 8, secondaryColor
    );

    // Glowing gem in center of headband
    DrawCircle(
        position.x,
        position.y - height - width/2 * 0.5f,
        width * 0.1f,
        PURPLE
    );

    // Face - mysterious glowing eyes
    float eyeSize = width * 0.15f;
    float eyeX = position.x - (facingLeft ? -eyeSize : eyeSize);

    // Glowing eyes
    Color eyeColor = {180, 100, 255, 255}; // Purple glow
    DrawCircle(
        eyeX,
        position.y - height - width/2 * 0.8f + width * 0.25f,
        eyeSize/2,
        eyeColor
    );

    // No visible mouth - mysterious

    // Energy effects during special attacks
    if (currentAnimation.find("special") != std::string::npos) {
        // Energy orb for projectile
        if (currentAnimation == "nspecial") {
            float orbSize = width * 0.3f * (1.0f + sin(GetTime() * 5) * 0.2f);
            float orbX = position.x + (facingLeft ? -width*0.8f : width*0.8f);

            DrawCircle(orbX, position.y - height * 0.5f, orbSize, {180, 100, 255, 180});
            DrawCircleLines(orbX, position.y - height * 0.5f, orbSize * 1.2f, {220, 140, 255, 150});
        }

        // Energy aura
        float time = GetTime() * 3.0f;
        for (int i = 0; i < 8; i++) {
            float angle = time + i * 45.0f * DEG2RAD;
            float distance = width * 0.8f * (0.8f + sin(time * 2.0f + i) * 0.2f);
            Vector2 pos = {
                position.x + cos(angle) * distance,
                position.y - height/2 + sin(angle) * distance
            };

            DrawCircle(pos.x, pos.y, 5.0f, {180, 100, 255, 150});
        }
    }
}

void CharacterVisuals::drawBrawlerCharacter(Vector2 position, float width, float height, float damageGlow) {
    // Mix color with damage glow
    Color drawColor = mainColor;
    drawColor.r = Lerp(mainColor.r, 255, damageGlow);
    drawColor.g = Lerp(mainColor.g, 100, damageGlow);
    drawColor.b = Lerp(mainColor.b, 100, damageGlow);

    // Body - more athletic and rounded
    DrawRectangleRounded(
        {position.x - width/2, position.y - height, width, height},
        0.3f, 8, drawColor
    );

    // Head - slightly larger for cartoon look
    DrawRectangleRounded(
        {position.x - width/2 * 0.8f, position.y - height - width/2 * 0.8f, width * 0.8f, width * 0.8f},
        0.5f, 8, drawColor
    );

    // Face - draw based on animation state
    float eyeSize = width * 0.15f;
    float eyeX = position.x - (facingLeft ? -eyeSize : eyeSize);

    // Eyes
    DrawRectangleRounded(
        {eyeX - eyeSize/2, position.y - height - width/2 * 0.8f + width * 0.25f, eyeSize, eyeSize},
        0.8f, 8, BLACK
    );

    // If in hitstun, show shocked expression
    if (currentAnimation == "hitstun") {
        // Shocked mouth
        DrawRectangleRounded(
            {position.x - width * 0.2f, position.y - height - width/2 * 0.8f + width * 0.5f, width * 0.4f, width * 0.15f},
            0.8f, 8, BLACK
        );
    } else {
        // Normal mouth - smile
        DrawRectangleRounded(
            {position.x - width * 0.3f, position.y - height - width/2 * 0.8f + width * 0.5f, width * 0.6f, width * 0.1f},
            0.8f, 8, BLACK
        );
    }

    // Boxing gloves for attacks
    if (currentAnimation.find("jab") != std::string::npos ||
        currentAnimation.find("smash") != std::string::npos) {

        // Lead hand for punch
        float handSize = width * 0.4f;
        float handX = position.x + (facingLeft ? -width/2 - handSize : width/2);
        float handY = position.y - height * 0.5f;

        // Draw boxing glove
        DrawRectangleRounded(
            {handX, handY, handSize, handSize},
            0.8f, 8, secondaryColor
        );
    }

    // Boxing cap/hat
    DrawRectangleRounded(
        {position.x - width/2 * 0.8f, position.y - height - width/2 * 0.8f - 5, width * 0.8f, 10},
        0.3f, 8, secondaryColor
    );

    // Add boxing shorts
    DrawRectangleRounded(
        {position.x - width/2 * 0.8f, position.y - height * 0.4f, width * 0.8f, height * 0.2f},
        0.3f, 4, secondaryColor
    );
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
