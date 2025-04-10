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
      isActive(true)
{
}

void AttackBox::update()
{
    // Future update mechanics can be implemented here
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