#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

// Game dimensions
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

// Physics constants
const float GRAVITY = 0.5f;
const float FAST_FALL_GRAVITY = 0.8f;
const float JUMP_FORCE = -14.0f;
const float DOUBLE_JUMP_FORCE = -10.0f;

// Smash mechanic constants
const float KNOCKBACK_BASE = 5.0f;
const float KNOCKBACK_SCALING = 0.1f;
const float DAMAGE_SCALING = 0.05f; // How much damage affects knockback
const int DEFAULT_STOCKS = 3; // Default number of lives
const float MAX_DAMAGE = 999.0f; // Max damage percentage
const float EXPLOSION_DAMAGE_THRESHOLD = 200.0f; // Damage at which characters explode

// Blast zones (stage boundaries)
const float BLAST_ZONE_LEFT = -300.0f;
const float BLAST_ZONE_RIGHT = SCREEN_WIDTH + 300.0f;
const float BLAST_ZONE_TOP = -300.0f;
const float BLAST_ZONE_BOTTOM = SCREEN_HEIGHT + 200.0f;

// Shield mechanics
const float MAX_SHIELD_HEALTH = 100.0f;
const float SHIELD_REGEN_RATE = 0.2f;
const float SHIELD_DAMAGE_MULTIPLIER = 0.7f; // Shield takes less damage than character
const int SHIELD_STUN_FRAMES = 4; // Frames of lag when shield is hit
const int SHIELD_BREAK_STUN = 180; // Stun frames when shield breaks (3 seconds)

// Dodge mechanics
const int SPOT_DODGE_FRAMES = 18;
const int ROLL_DODGE_FRAMES = 24;
const int AIR_DODGE_FRAMES = 30;
const int DODGE_COOLDOWN = 40; // Frames before can dodge again
const int DODGE_INVINCIBLE_START = 3;
const int DODGE_INVINCIBLE_END = 15;

// Ledge mechanics
const int LEDGE_HANG_MAX = 180; // Max frames character can hang on ledge
const int LEDGE_INVINCIBLE = 30; // Invincibility frames after grabbing ledge
const int LEDGE_COOLDOWN = 60; // Frames before can grab same ledge again

// Hitstun and knockback
const float HITSTUN_MULTIPLIER = 0.4f; // Hitstun frames = knockback * multiplier

// Game state timers
const int RESPAWN_TIME = 120; // Frames before respawning (2 seconds)
const int GAME_START_TIMER = 180; // 3 second countdown at start
const int GAME_END_DELAY = 180; // 3 second pause at game end

// UI constants
const int DAMAGE_FONT_SIZE = 32;
const int STOCK_ICON_SIZE = 32;
const int HUD_MARGIN = 20;

#endif // GAME_CONFIG_H
