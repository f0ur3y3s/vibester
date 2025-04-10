#ifndef CHARACTER_CONFIG_H
#define CHARACTER_CONFIG_H

#include "raylib.h"
#include <string>

// Game constants moved to a central configuration file
namespace GameConfig
{
    // Screen dimensions
    constexpr int SCREEN_WIDTH = 1280;
    constexpr int SCREEN_HEIGHT = 720;

    // Blast zones
    constexpr float BLAST_ZONE_LEFT = -200.0f;
    constexpr float BLAST_ZONE_RIGHT = SCREEN_WIDTH + 200.0f;
    constexpr float BLAST_ZONE_TOP = -200.0f;
    constexpr float BLAST_ZONE_BOTTOM = SCREEN_HEIGHT + 200.0f;

    // Physics constants
    constexpr float GRAVITY = 0.5f;
    constexpr float FAST_FALL_GRAVITY = 0.8f;
    constexpr float JUMP_FORCE = -12.0f;
    constexpr float DOUBLE_JUMP_FORCE = -10.0f;
    constexpr float GROUND_FRICTION = 0.45f; // Adjust this value as needed

    // Character constants
    constexpr int DEFAULT_STOCKS = 3;
    constexpr float MAX_DAMAGE = 999.0f;
    constexpr float MAX_SHIELD_HEALTH = 100.0f;
    constexpr float SHIELD_REGEN_RATE = 0.2f;
    constexpr float SHIELD_DAMAGE_MULTIPLIER = 0.7f;
    constexpr float DAMAGE_SCALING = 0.05f;
    constexpr float HITSTUN_MULTIPLIER = 0.5f;

    // Timing constants
    constexpr int SHIELD_BREAK_STUN = 180;
    constexpr int SHIELD_STUN_FRAMES = 5;
    constexpr int SPOT_DODGE_FRAMES = 20;
    constexpr int ROLL_DODGE_FRAMES = 25;
    constexpr int AIR_DODGE_FRAMES = 30;
    constexpr int DODGE_COOLDOWN = 45;
    constexpr int DODGE_INVINCIBLE_START = 3;
    constexpr int DODGE_INVINCIBLE_END = 15;
}

// Character configuration for different character types
struct CharacterConfig
{
    float width;
    float height;
    float speed;
    Color color;
    std::string name;

    // Customizable parameters
    float jumpForce;
    float doubleJumpForce;
    float weight;
    int specialNeutralCooldown;
    int specialSideCooldown;
    int specialUpCooldown;
    int specialDownCooldown;

    // Default configuration
    static CharacterConfig Default()
    {
        return {
            50.0f, // width
            80.0f, // height
            5.0f, // speed
            BLUE, // color
            "Character", // name
            GameConfig::JUMP_FORCE, // jumpForce
            GameConfig::DOUBLE_JUMP_FORCE, // doubleJumpForce
            1.0f, // weight
            120, // specialNeutralCooldown
            90, // specialSideCooldown
            60, // specialUpCooldown
            120 // specialDownCooldown
        };
    }
};

#endif // CHARACTER_CONFIG_H
