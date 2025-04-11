#ifndef CHARACTER_VISUALS_H
#define CHARACTER_VISUALS_H

#include "raylib.h"
#include "StateManager.h"
#include <string>
#include <unordered_map>
#include <vector>

class Character;

// Animation frame data
struct AnimationFrame
{
    Rectangle sourceRect; // Source rectangle from spritesheet
    float duration; // Frame duration in seconds
    Vector2 offset; // Offset to apply to character position
    Vector2 hitboxOffset; // Offset for attack hitbox (if applicable)
    Vector2 hitboxSize; // Size of attack hitbox (if applicable)
    bool isHitActive; // Whether this frame has an active hit
};

// Animation data
struct Animation
{
    std::vector<AnimationFrame> frames;
    bool loops; // Whether animation loops
    float totalDuration; // Total duration of animation
    int currentFrame; // Current frame index
    float timer; // Current frame timer

    Animation() : loops(false), totalDuration(0), currentFrame(0), timer(0)
    {
    }

    void update(float deltaTime, bool facingLeft)
    {
        timer += deltaTime;

        // Check if we should advance to next frame
        if (timer >= frames[currentFrame].duration)
        {
            timer -= frames[currentFrame].duration;
            currentFrame++;

            // Handle looping or end of animation
            if (currentFrame >= frames.size())
            {
                if (loops)
                {
                    currentFrame = 0;
                }
                else
                {
                    currentFrame = frames.size() - 1; // Stay on last frame
                }
            }
        }
    }

    // Get current frame data
    AnimationFrame& getCurrentFrame()
    {
        return frames[currentFrame];
    }

    // Reset animation
    void reset()
    {
        currentFrame = 0;
        timer = 0.0f;
    }

    // Check if animation is finished
    bool isFinished() const
    {
        return !loops && currentFrame == frames.size() - 1 && timer >= frames[currentFrame].duration;
    }
};

// Character visual style
enum CharacterStyle
{
    STYLE_BRAWLER, // Like Mario, balanced fighter
    STYLE_SPEEDY, // Like Fox, fast but light
    STYLE_HEAVY, // Like Bowser, strong but slow
    STYLE_SWORD, // Like Link, uses weapons
    STYLE_CUSTOM // Custom style
};

// Visual effects
struct VisualEffect
{
    Vector2 position;
    float lifeSpan;
    float currentLife;
    float scale;
    float rotation;
    Color color;
    int effectType; // 0 = hit spark, 1 = dust, 2 = shield, 3 = smash charge, etc.

    bool update(float deltaTime)
    {
        currentLife -= deltaTime;
        return currentLife > 0;
    }
};

// CharacterVisuals class to handle all visual aspects
class CharacterVisuals
{
private:
    Character* owner; // Pointer to owner character
    Texture2D spriteSheet; // Character sprite sheet
    std::unordered_map<std::string, Animation> animations; // Map of animations by name
    std::string currentAnimation; // Current animation name
    bool facingLeft; // Direction character is facing
    float visualScale; // Visual scale (can be different from hitbox)
    CharacterStyle style; // Character's visual style
    Color mainColor; // Primary character color
    Color secondaryColor; // Secondary character color
    Color effectColor; // Special effect color

    std::vector<VisualEffect> effects; // Active visual effects

    // Shader effects
    Shader outlineShader; // Outline shader for character
    Shader smashChargeShader; // Smash attack charge shader
    bool useShaders; // Whether to use shaders

    // Trail effect for fast movements
    struct TrailPoint
    {
        Vector2 position;
        float alpha;
    };

    std::vector<TrailPoint> movementTrail; // Movement trail points

    // Particle system
    struct Particle
    {
        Vector2 position;
        Vector2 velocity;
        Color color;
        float size;
        float life;
    };

    std::vector<Particle> particles; // Active particles

public:
    CharacterVisuals(Character* character, CharacterStyle characterStyle, Color primary, Color secondary);
    ~CharacterVisuals();

    // Initialize the visuals system
    static void InitShaders();

    // Load appropriate sprite sheet based on style
    void loadSpriteSheet();

    // Setup all animations for this character
    void setupAnimations();

    // Setup animations common to all characters
    void setupCommonAnimations();

    // Style-specific animation setups
    void setupBrawlerAnimations();
    void setupSpeedyAnimations();
    void setupHeavyAnimations();
    void setupSwordAnimations();
    void setupCustomAnimations();

    // Set current animation
    void setAnimation(const std::string& animName);

    // Update visuals
    void update(float deltaTime, bool isFacingLeft);

    // Update movement trail
    void updateMovementTrail(float deltaTime);

    // Update particles
    void updateParticles(float deltaTime);

    // Add trail point
    void addTrailPoint(Vector2 position);

    // Add hit effect
    void addHitEffect(Vector2 position, float size, Color color);

    // Add dust effect
    void addDustEffect(Vector2 position);

    // Add shield effect
    void addShieldEffect(Vector2 position, float size, Color color);

    // Add hit particles
    void addHitParticles(Vector2 position, int count, Color color);

    // Draw character
    void draw(Vector2 position, float width, float height, float damage);

    // Draw movement trail
    void drawMovementTrail(Vector2 position);

    // Draw particles
    void drawParticles();

    // Map attack type to animation
    void updateAnimationFromAttack(AttackType attackType);

    // Map character state to animation
    void updateAnimationFromState(CharacterState state, bool isAttacking, bool isGrabbing);

    // Get current animation name
    const std::string& getCurrentAnimation() const;

    // Add smash charge effect
    void addSmashChargeEffect(Vector2 position, float power);

    // Add methods for visualizing death and explosion animations
    void drawDeathAnimation(Vector2 position, float width, float height, float rotation, float scale, float damage);
    void drawExplosionEffect(Vector2 position, int frame, int totalFrames);

    void drawCustomCharacter(Vector2 position, float width, float height, float damageGlow);
    void setupPlaceholderAnimations();
    void drawSpeedyCharacter(Vector2 position, float width, float height, float damageGlow);
    void drawHeavyCharacter(Vector2 position, float width, float height, float damageGlow);
    void drawSwordCharacter(Vector2 position, float width, float height, float damageGlow);
    void drawBrawlerCharacter(Vector2 position, float width, float height, float damageGlow);
};

#endif // CHARACTER_VISUALS_H
