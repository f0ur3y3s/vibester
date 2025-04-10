#include "../../include/attacks/AttackBox.h"

AttackBox::AttackBox(
    Rectangle r, 
    float dmg, 
    float baseKb, 
    float kbScaling, 
    float kbAngle, 
    int lag, 
    int shield
) 
    : rect(r),
      damage(dmg),
      baseKnockback(baseKb),
      knockbackScaling(kbScaling),
      knockbackAngle(kbAngle),
      hitLag(lag),
      shieldStun(shield),
      type(NORMAL),
      isActive(true),
      duration(10),
      currentFrame(0),
      velocity({0, 0}),
      destroyOnHit(false)
{
}

AttackBox::AttackBox(
    Rectangle r, 
    float dmg, 
    float baseKb, 
    float kbScaling, 
    float kbAngle, 
    int lag, 
    int dur,
    Vector2 vel, 
    bool destroy
) 
    : rect(r),
      damage(dmg),
      baseKnockback(baseKb),
      knockbackScaling(kbScaling),
      knockbackAngle(kbAngle),
      hitLag(lag),
      shieldStun(0),
      type(PROJECTILE),
      isActive(true),
      duration(dur),
      currentFrame(0),
      velocity(vel),
      destroyOnHit(destroy)
{
}

bool AttackBox::update()
{
    // For projectiles, update position
    if (type == PROJECTILE) {
        rect.x += velocity.x;
        rect.y += velocity.y;
    }

    // Check duration
    currentFrame++;
    if (currentFrame >= duration) {
        isActive = false;
        return false;
    }

    return true;
}

void AttackBox::draw(bool debug)
{
    if (debug && isActive) {
        // Draw hitbox visualization in debug mode
        Color hitboxColor = {255, 0, 0, 128};
        
        // Different colors for different hitbox types
        if (type == GRAB) {
            hitboxColor = {0, 0, 255, 128}; // Blue for grabs
        } else if (type == PROJECTILE) {
            hitboxColor = {255, 255, 0, 128}; // Yellow for projectiles
        }
        
        DrawRectangleRec(rect, hitboxColor);
        DrawRectangleLinesEx(rect, 1.0f, RED);
    }
}