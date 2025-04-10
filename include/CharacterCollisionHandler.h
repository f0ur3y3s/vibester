#ifndef CHARACTER_COLLISION_HANDLER_H
#define CHARACTER_COLLISION_HANDLER_H

#include "Platform.h"
#include <vector>

// Forward declaration to avoid circular dependency
class Character;

class CharacterCollisionHandler {
public:
    CharacterCollisionHandler(Character& character) : character(character) {}
    
    // Handle all platform collisions
    // Returns true if character is standing on ground
    bool handlePlatformCollisions(std::vector<Platform>& platforms);
    
    // Check for collision with a specific platform
    // Returns true if collision was handled
    bool checkPlatformCollision(const Platform& platform, Rectangle playerRect, 
                               float stepX, float stepY);
                               
    // Handle blast zone collision (out of bounds)
    bool checkBlastZoneCollision();
    
    // Check for attack hitbox collisions between characters
    bool checkAttackCollision(Character& other);
    
    // Check if character is standing on any platform
    bool isOnGround(const std::vector<Platform>& platforms);
    
private:
    Character& character;
    
    // Platform collision handlers
    bool handleSolidPlatform(const Platform& platform, Rectangle playerRect, 
                            float stepX, float stepY);
    bool handlePassthroughPlatform(const Platform& platform, Rectangle playerRect, 
                                  float stepX, float stepY);
};

#endif // CHARACTER_COLLISION_HANDLER_H