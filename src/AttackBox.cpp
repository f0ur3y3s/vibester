// #include "AttackBox.h"
//
// #include <cstdio>
// #include <math.h>
//
// // Constructor for standard attack
// AttackBox::AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling,
//                      float kbAngle, int hitstun, int dur) {
//     rect = r;
//     damage = dmg;
//     baseKnockback = baseKB;
//     knockbackScaling = kbScaling;
//     knockbackGrowth = 1.0f;
//     knockbackAngle = kbAngle;
//     hitstunFrames = hitstun;
//     canSpike = false;
//     ignoresShield = false;
//     shieldStun = 0;
//     duration = dur;
//     currentFrame = 0;
//     isActive = true;
//
//     // Effects
//     causesFreeze = false;
//     freezeFrames = 0;
//     launchesUpward = false;
//
//     // Type
//     type = NORMAL;
// }
//
// // Constructor for projectile
// AttackBox::AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling,
//                      float kbAngle, int hitstun, int dur, Vector2 vel, bool destroy) {
//     rect = r;
//     damage = dmg;
//     baseKnockback = baseKB;
//     knockbackScaling = kbScaling;
//     knockbackGrowth = 1.0f;
//     knockbackAngle = kbAngle;
//     hitstunFrames = hitstun;
//     canSpike = false;
//     ignoresShield = false;
//     shieldStun = 0;
//     duration = dur;
//     currentFrame = 0;
//     isActive = true;
//
//     // Projectile specific
//     velocity = vel;
//     destroyOnHit = destroy;
//
//     // Effects
//     causesFreeze = false;
//     freezeFrames = 0;
//     launchesUpward = false;
//
//     // Type
//     type = PROJECTILE;
// }
//
// // Constructor for special hitbox types
// AttackBox::AttackBox(Rectangle r, float dmg, float baseKB, float kbScaling,
//                      float kbAngle, int hitstun, int dur, HitboxType hitType) {
//     rect = r;
//     damage = dmg;
//     baseKnockback = baseKB;
//     knockbackScaling = kbScaling;
//     knockbackGrowth = 1.0f;
//     knockbackAngle = kbAngle;
//     hitstunFrames = hitstun;
//     canSpike = false;
//     ignoresShield = false;
//     shieldStun = 0;
//     duration = dur;
//     currentFrame = 0;
//     isActive = true;
//
//     // Special type
//     type = hitType;
//
//     // Special properties based on type
//     switch (type) {
//         case REFLECTOR:
//             ignoresShield = true;
//             break;
//
//         case GRAB:
//         case COMMAND_GRAB:
//             damage = 0;
//             baseKnockback = 0;
//             hitstunFrames = 60; // Grab duration
//
//             // Command grab ignores shields
//             if (type == COMMAND_GRAB) {
//                 ignoresShield = true;
//             }
//             break;
//
//         case WINDBOX:
//             damage = 0; // No damage, just knockback
//             break;
//
//         default:
//             break;
//     }
//
//     // Effects
//     causesFreeze = false;
//     freezeFrames = 0;
//     launchesUpward = false;
// }
//
// bool AttackBox::update() {
//     currentFrame++;
//
//     // For projectiles, update position
//     if (type == PROJECTILE) {
//         rect.x += velocity.x;
//         rect.y += velocity.y;
//     }
//
//     // Check if hitbox has expired
//     if (currentFrame >= duration) {
//         isActive = false;
//         return false;
//     }
//
//     return true;
// }
//
// void AttackBox::updatePosition(Vector2 ownerPos, bool isFacingRight) {
//     // Skip for projectiles, as they move independently
//     if (type == PROJECTILE) return;
//
//     // Position the attack box relative to the character
//     float offsetX = isFacingRight ? 1.0f : -1.0f;
//
//     // Calculate new position based on original offset
//     float relativeX = rect.width / 2 * offsetX;
//     float boxCenterX = ownerPos.x + relativeX;
//
//     // Update position
//     rect.x = boxCenterX - (rect.width / 2);
//     rect.y = ownerPos.y - (rect.height / 2);
// }
//
// void AttackBox::draw(bool debugMode) {
//     if (!isActive) return;
//
//     // Only draw hitboxes in debug mode
//     if (debugMode) {
//         Color hitboxColor;
//
//         // Different colors for different hitbox types
//         switch (type) {
//             case PROJECTILE:
//                 hitboxColor = {255, 0, 255, 128}; // Purple
//                 break;
//
//             case GRAB:
//             case COMMAND_GRAB:
//                 hitboxColor = {0, 255, 255, 128}; // Cyan
//                 break;
//
//             case REFLECTOR:
//                 hitboxColor = {0, 255, 0, 128}; // Green
//                 break;
//
//             case ABSORBER:
//                 hitboxColor = {255, 255, 0, 128}; // Yellow
//                 break;
//
//             case WINDBOX:
//                 hitboxColor = {200, 200, 255, 128}; // Light blue
//                 break;
//
//             case NORMAL:
//             default:
//                 hitboxColor = {255, 0, 0, 128}; // Red
//                 break;
//         }
//
//         // Draw rectangle with semi-transparency
//         DrawRectangleRec(rect, hitboxColor);
//
//         // Draw outline
//         DrawRectangleLinesEx(rect, 1.0f, {255, 255, 255, 200});
//
//         // Draw damage value
//         char damageText[10];
//         sprintf(damageText, "%.1f", damage);
//         DrawText(
//             damageText,
//             static_cast<int>(rect.x + rect.width/2 - 10),
//             static_cast<int>(rect.y + rect.height/2 - 10),
//             16,
//             WHITE
//         );
//     }
// }
//
// Vector2 AttackBox::calculateKnockback(float targetDamage, float chargeTime) {
//     // Calculate knockback magnitude using Smash-style formula
//     float chargeMultiplier = 1.0f + (chargeTime / 60.0f) * knockbackGrowth;
//     float knockbackMagnitude = (baseKnockback + (damage * targetDamage * 0.05f * knockbackScaling)) * chargeMultiplier;
//
//     // Convert angle to radians
//     float angleRad = knockbackAngle * DEG2RAD;
//
//     // Calculate knockback vector
//     Vector2 knockbackVector;
//     knockbackVector.x = cosf(angleRad) * knockbackMagnitude;
//     knockbackVector.y = sinf(angleRad) * knockbackMagnitude;
//
//     // Special case for spikes
//     if (canSpike && knockbackAngle > 180.0f) {
//         // Force downward knockback for spikes
//         knockbackVector.y = abs(knockbackVector.y);
//     }
//
//     // Special case for upward launchers
//     if (launchesUpward) {
//         knockbackVector.y = -abs(knockbackVector.y);
//     }
//
//     return knockbackVector;
// }