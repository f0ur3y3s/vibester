#include "attacks/AttackBox.h"

// Constructor for standard attack
AttackBox::AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur)
{
    rect = r;
    damage = dmg;
    baseKnockback = baseKB;
    knockbackScaling = kbScaling;
    knockbackAngle = kbAngle;
    hitstunFrames = hitstun;
    duration = dur;
    currentFrame = 0;
    type = NORMAL;
    canSpike = false;
    shieldStun = 0;
    ignoresShield = false;
    causesFreeze = false;
    freezeFrames = 0;
    launchesUpward = false;
    knockbackGrowth = 0.0f;
    isActive = true;
    velocity = {0, 0};
    destroyOnHit = false;
}

// Constructor for projectile
AttackBox::AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur,
                     Vector2 vel, bool destroy)
{
    rect = r;
    damage = dmg;
    baseKnockback = baseKB;
    knockbackScaling = kbScaling;
    knockbackAngle = kbAngle;
    hitstunFrames = hitstun;
    duration = dur;
    currentFrame = 0;
    type = PROJECTILE;
    canSpike = false;
    shieldStun = 0;
    ignoresShield = false;
    causesFreeze = false;
    freezeFrames = 0;
    launchesUpward = false;
    knockbackGrowth = 0.0f;
    isActive = true;
    velocity = vel;
    destroyOnHit = destroy;
}

// Constructor for special hitbox types
AttackBox::AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling, float kbAngle, int hitstun, int dur,
                     BoxType hitType)
{
    rect = r;
    damage = dmg;
    baseKnockback = baseKB;
    knockbackScaling = kbScaling;
    knockbackAngle = kbAngle;
    hitstunFrames = hitstun;
    duration = dur;
    currentFrame = 0;
    type = hitType;
    canSpike = false;
    shieldStun = 0;
    ignoresShield = (type == COMMAND_GRAB);
    causesFreeze = false;
    freezeFrames = 0;
    launchesUpward = false;
    knockbackGrowth = 0.0f;
    isActive = true;
    velocity = {0, 0};
    destroyOnHit = false;
}

// Methods
bool AttackBox::update()
{
    // For projectiles, update position
    if (type == PROJECTILE)
    {
        rect.x += velocity.x;
        rect.y += velocity.y;
    }

    currentFrame++;
    if (currentFrame >= duration)
    {
        isActive = false;
        return false;
    }

    return true;
}

void AttackBox::updatePosition(Vector2 ownerPos, bool isFacingRight)
{
    if (type == PROJECTILE)
    {
        // Projectiles move independently once created
        return;
    }

    // Calculate offset based on facing direction
    float offsetX = isFacingRight ? 1.0f : -1.0f;
    float boxWidth = rect.width;

    // For attacks that face the character's direction
    if (isFacingRight)
    {
        // Right-facing position
        rect.x = ownerPos.x + boxWidth / 4;
    }
    else
    {
        // Left-facing position - need to position to the left of character
        rect.x = ownerPos.x - boxWidth - boxWidth / 4;
    }

    // Center vertically
    rect.y = ownerPos.y - rect.height / 2;
}

void AttackBox::draw(bool debugMode = false)
{
    if (!debugMode) return;

    Color hitboxColor;
    switch (type)
    {
    case GRAB:
    case COMMAND_GRAB:
        hitboxColor = {255, 100, 255, 128}; // Purple
        break;
    case PROJECTILE:
        hitboxColor = {0, 255, 255, 128}; // Cyan
        break;
    case REFLECTOR:
        hitboxColor = {0, 200, 255, 128}; // Blue
        break;
    case ABSORBER:
        hitboxColor = {0, 255, 200, 128}; // Teal
        break;
    case WINDBOX:
        hitboxColor = {200, 255, 200, 128}; // Light green
        break;
    default:
        hitboxColor = {255, 0, 0, 128}; // Red
        break;
    }

    DrawRectangleRec(rect, hitboxColor);
    DrawRectangleLinesEx(rect, 2, {255, 255, 255, 200});
}
