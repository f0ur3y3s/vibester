#include "raylib.h"
#include "Character.h"
#include "Platform.h"
#include "Particle.h"
#include "ParticleSystem.h"
#include "Constants.h"
#include "Item.h"
#include "GameState.h"
#include <vector>
#include <string>
#include <iostream>
#include <cmath> // For fabs function
#include <algorithm> // For std::min, std::max

// AI state machine parameters
struct AIState {
    enum State {
        APPROACH,       // Move toward player
        ATTACK,         // Execute attacks when in range
        DEFEND,         // Shield or dodge when player attacks
        RECOVER,        // Return to stage when off-stage
        RETREAT,        // Back away to reset neutral game
        EDGE_GUARD      // Attempt to prevent player recovery
    };

    State currentState;
    int stateTimer;
    int decisionDelay;
    Vector2 targetPosition;
    int reactionTime;
    int lastAttackFrame;
    bool wasPlayerAttacking;
    float threatLevel;
    bool isOffStage;
    bool playerIsOffStage;
    float lastDistanceX;
    float lastDistanceY;
    int adaptiveTimer;
    bool comboState;
    int comboCounter;

    AIState() {
        currentState = APPROACH;
        stateTimer = 0;
        decisionDelay = 5;  // Expert AI makes decisions faster
        reactionTime = 3;   // Fast reaction time
        lastAttackFrame = 0;
        wasPlayerAttacking = false;
        threatLevel = 0.0f;
        isOffStage = false;
        playerIsOffStage = false;
        lastDistanceX = 0.0f;
        lastDistanceY = 0.0f;
        adaptiveTimer = 0;
        comboState = false;
        comboCounter = 0;
    }
};

// Global AI state
AIState aiState;

// Main game functions
void InitGame();
void UpdateGame();
void DrawGame();
void CleanupGame();

// AI Functions
void UpdateEnemyAI();
bool IsOffStage(Vector2 position);
void UpdateThreatLevel(Character* player, float absDistanceX, float absDistanceY);
void DetermineAIState(float absDistanceX, float absDistanceY);
void ExecuteApproachBehavior(float distanceX, float distanceY, float absDistanceX, float absDistanceY);
void ExecuteAttackBehavior(float distanceX, float distanceY, float absDistanceX, float absDistanceY);
void ExecuteDefendBehavior(float distanceX, float distanceY);
void ExecuteRecoverBehavior(float distanceX, float absDistanceX);
void ExecuteRetreatBehavior(float distanceX);
void ExecuteEdgeGuardBehavior(Vector2 playerPos, Vector2 enemyPos);

// Global game variables
GameState gameState;
std::vector<Character*> players;
std::vector<Platform> platforms;
std::vector<Vector2> spawnPoints;
std::vector<Particle> particles;
Font gameFont;
bool debugMode = false;

// Main entry point
int main() {
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Super Smash Clone - Expert AI Mode");
    SetTargetFPS(60);

    // Initialize game
    InitGame();

    // Main game loop
    while (!WindowShouldClose()) {
        UpdateGame();

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawGame();

        EndDrawing();
    }

    // Cleanup
    CleanupGame();
    CloseWindow();

    return 0;
}

void InitGame() {
    // Load font
    gameFont = GetFontDefault();

    // Create platforms (battlefield style)
    // Main platform
    platforms.push_back(Platform(
        SCREEN_WIDTH/2 - 300, SCREEN_HEIGHT - 100,
        600, 50,
        DARKGRAY
    ));

    // Side platforms
    platforms.push_back(Platform(
        SCREEN_WIDTH/2 - 250, SCREEN_HEIGHT - 250,
        150, 20,
        GRAY
    ));

    platforms.push_back(Platform(
        SCREEN_WIDTH/2 + 100, SCREEN_HEIGHT - 250,
        150, 20,
        GRAY
    ));

    // Top platform
    platforms.push_back(Platform(
        SCREEN_WIDTH/2 - 75, SCREEN_HEIGHT - 400,
        150, 20,
        GRAY
    ));

    // Create spawn points
    spawnPoints.push_back({SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT - 200});
    spawnPoints.push_back({SCREEN_WIDTH/2 + 200, SCREEN_HEIGHT - 200});
    spawnPoints.push_back({SCREEN_WIDTH/2, SCREEN_HEIGHT - 200});
    spawnPoints.push_back({SCREEN_WIDTH/2, SCREEN_HEIGHT - 300});

    // Create player and enemy
    Character* player1 = new Character(
        spawnPoints[0].x, spawnPoints[0].y,
        50, 80,
        5.0f,
        RED,
        "Player 1"
    );

    Character* enemy = new Character(
        spawnPoints[1].x, spawnPoints[1].y,
        50, 80,
        5.0f,
        BLUE,
        "Enemy"
    );

    players.push_back(player1);
    players.push_back(enemy);

    // Initialize game state
    gameState = GameState();
    gameState.players = players;
    gameState.platforms = platforms;
    gameState.spawnPoints = spawnPoints;

    // Start in title screen
    gameState.currentState = GameState::TITLE_SCREEN;

    // Setup default match settings
    gameState.settings.stockCount = DEFAULT_STOCKS;
    gameState.settings.timeLimit = 180; // 3 minutes
    gameState.settings.itemsEnabled = true;
    gameState.settings.itemFrequency = 0.5f;
    gameState.settings.stageHazards = true;
    gameState.settings.finalSmash = true;
}

void UpdateGame() {
    // Process game state
    switch (gameState.currentState) {
        case GameState::TITLE_SCREEN:
            // Title screen logic
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                gameState.changeState(GameState::GAME_START);
            }
            break;

        case GameState::CHARACTER_SELECT:
            // Character selection logic
            if (IsKeyPressed(KEY_ENTER)) {
                gameState.changeState(GameState::STAGE_SELECT);
            }
            break;

        case GameState::STAGE_SELECT:
            // Stage selection logic
            if (IsKeyPressed(KEY_ENTER)) {
                gameState.changeState(GameState::GAME_START);
            }
            break;

        case GameState::GAME_START:
            // Game start countdown
            gameState.stateTimer++;
            if (gameState.stateTimer >= GAME_START_TIMER) {
                gameState.changeState(GameState::GAME_PLAYING);
            }
            break;

        case GameState::GAME_PLAYING:
            // Check for pause
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                gameState.pauseGame();
                break;
            }

            // Update debug mode
            if (IsKeyPressed(KEY_F1)) {
                debugMode = !debugMode;
            }

            // Update players
            for (auto& player : players) {
                player->update(platforms);
            }

            // Check for character collisions for attacks
            for (auto& attacker : players) {
                if (attacker->isAttacking) {
                    for (auto& defender : players) {
                        if (attacker != defender) {
                            attacker->checkHit(*defender);
                        }
                    }
                }
            }

            // Update particles
            for (int i = 0; i < particles.size(); i++) {
                if (!particles[i].update()) {
                    particles.erase(particles.begin() + i);
                    i--;
                }
            }

            // Player 1 controls
            if (players[0]->stocks > 0 && !players[0]->isDying) {
                // Movement
                if (IsKeyDown(KEY_A)) players[0]->moveLeft();
                if (IsKeyDown(KEY_D)) players[0]->moveRight();
                if (IsKeyPressed(KEY_W)) players[0]->jump();
                if (IsKeyDown(KEY_S)) players[0]->fastFall();

                // Attacks
                if (IsKeyPressed(KEY_J)) {
                    // Basic attack - context sensitive
                    if (players[0]->state == Character::JUMPING || players[0]->state == Character::FALLING) {
                        players[0]->neutralAir();
                    } else {
                        players[0]->jab();
                    }
                }

                if (IsKeyPressed(KEY_K)) {
                    // Special attack - context sensitive
                    if (IsKeyDown(KEY_A)) {
                        // Side special left
                        players[0]->sideSpecial();
                    } else if (IsKeyDown(KEY_D)) {
                        // Side special right
                        players[0]->sideSpecial();
                    } else if (IsKeyDown(KEY_W)) {
                        // Up special
                        players[0]->upSpecial();
                    } else if (IsKeyDown(KEY_S)) {
                        // Down special
                        players[0]->downSpecial();
                    } else {
                        // Neutral special
                        players[0]->neutralSpecial();
                    }
                }

                // Smash attacks
                if (IsKeyDown(KEY_L)) {
                    if (IsKeyDown(KEY_A)) {
                        // Forward smash left
                        players[0]->forwardSmash(20.0f);
                    } else if (IsKeyDown(KEY_D)) {
                        // Forward smash right
                        players[0]->forwardSmash(20.0f);
                    } else if (IsKeyDown(KEY_W)) {
                        // Up smash
                        players[0]->upSmash(20.0f);
                    } else if (IsKeyDown(KEY_S)) {
                        // Down smash
                        players[0]->downSmash(20.0f);
                    }
                }

                // Shield/Dodge
                if (IsKeyDown(KEY_I)) {
                    if (IsKeyPressed(KEY_A)) {
                        players[0]->forwardDodge();
                    } else if (IsKeyPressed(KEY_D)) {
                        players[0]->backDodge();
                    } else if (IsKeyPressed(KEY_S)) {
                        players[0]->spotDodge();
                    } else {
                        players[0]->shield();
                    }
                } else if (IsKeyReleased(KEY_I)) {
                    players[0]->releaseShield();
                }

                // Grab
                if (IsKeyPressed(KEY_U)) {
                    players[0]->grab();
                }

                // Throws (when grabbing)
                if (players[0]->isGrabbing) {
                    if (IsKeyPressed(KEY_J)) {
                        players[0]->pummel();
                    } else if (IsKeyPressed(KEY_A)) {
                        players[0]->backThrow();
                    } else if (IsKeyPressed(KEY_D)) {
                        players[0]->forwardThrow();
                    } else if (IsKeyPressed(KEY_W)) {
                        players[0]->upThrow();
                    } else if (IsKeyPressed(KEY_S)) {
                        players[0]->downThrow();
                    }
                }
            }

            // Run the expert AI for the enemy
            UpdateEnemyAI();

            // Check for game end conditions
            {
                bool allButOneDead = true;
                int aliveCount = 0;

                for (auto& player : players) {
                    if (player->stocks > 0) {
                        aliveCount++;
                    }
                }

                if (aliveCount <= 1) {
                    gameState.changeState(GameState::GAME_OVER);
                }
            }
            break;

        case GameState::GAME_PAUSED:
            // Pause menu logic
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
                gameState.resumeGame();
            }

            if (IsKeyPressed(KEY_R)) {
                gameState.resetMatch();
            }
            break;

        case GameState::GAME_OVER:
            // Game over logic
            gameState.stateTimer++;
            if (gameState.stateTimer >= GAME_END_DELAY) {
                gameState.changeState(GameState::RESULTS_SCREEN);
            }
            break;

        case GameState::RESULTS_SCREEN:
            // Results screen logic
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                gameState.resetMatch();
                gameState.changeState(GameState::TITLE_SCREEN);
            }
            break;

        default:
            break;
    }
}

void DrawGame() {
    // Draw background
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {135, 206, 235, 255}); // Sky blue

    // Draw platforms
    for (auto& platform : platforms) {
        platform.draw();
    }

    // Draw particles
    for (auto& particle : particles) {
        particle.draw();
    }

    // Draw players
    for (auto& player : players) {
        player->draw();
    }

    // Draw HUD
    for (int i = 0; i < players.size(); i++) {
        Color playerColor = players[i]->color;

        // Stock icons
        for (int s = 0; s < players[i]->stocks; s++) {
            DrawRectangle(
                HUD_MARGIN + s * (STOCK_ICON_SIZE + 5) + i * 200,
                HUD_MARGIN,
                STOCK_ICON_SIZE,
                STOCK_ICON_SIZE,
                playerColor
            );
        }

        // Damage percentage
        DrawText(
            TextFormat("P%d: %.0f%%", i+1, players[i]->damagePercent),
            HUD_MARGIN + i * 200,
            HUD_MARGIN + STOCK_ICON_SIZE + 5,
            DAMAGE_FONT_SIZE,
            playerColor
        );
    }

    // Draw state-specific screens
    switch (gameState.currentState) {
        case GameState::TITLE_SCREEN:
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});
            DrawText("SUPER SMASH CLONE - EXPERT AI MODE", SCREEN_WIDTH/2 - 280, SCREEN_HEIGHT/4, 40, WHITE);
            DrawText("Press ENTER to Start", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2, 30, WHITE);
            DrawText("Player Controls: WASD to move, J to attack, K for special, L for smash",
                     SCREEN_WIDTH/2 - 300, SCREEN_HEIGHT - 200, 20, WHITE);
            DrawText("Can you defeat the expert AI? Good luck!",
                     SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT - 170, 20, WHITE);
            break;

        case GameState::GAME_START:
            {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 100});
                int countdown = (GAME_START_TIMER - gameState.stateTimer) / 60 + 1;
                DrawText(TextFormat("%d", countdown), SCREEN_WIDTH/2 - 20, SCREEN_HEIGHT/2 - 50, 100, WHITE);
            }
            break;

        case GameState::GAME_PAUSED:
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});
            DrawText("PAUSED", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/3, 50, WHITE);
            DrawText("Press P to Resume", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2, 30, WHITE);
            DrawText("Press R to Restart", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 40, 30, WHITE);
            break;

        case GameState::GAME_OVER:
            {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});

                // Find the winner
                int winnerId = -1;
                for (int i = 0; i < players.size(); i++) {
                    if (players[i]->stocks > 0) {
                        winnerId = i;
                        break;
                    }
                }

                if (winnerId == 0) {
                    DrawText("YOU WIN!", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/3, 50, GREEN);
                } else if (winnerId == 1) {
                    DrawText("EXPERT AI WINS!", SCREEN_WIDTH/2 - 160, SCREEN_HEIGHT/3, 50, RED);
                } else {
                    DrawText("DRAW!", SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/3, 40, WHITE);
                }
            }
            break;

        case GameState::RESULTS_SCREEN:
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});
            DrawText("RESULTS", SCREEN_WIDTH/2 - 80, 100, 40, WHITE);

            for (int i = 0; i < players.size(); i++) {
                Color playerColor = players[i]->color;
                std::string displayName = (i == 0) ? "You" : "Expert AI";
                DrawText(displayName.c_str(), 200, 200 + i*80, 30, playerColor);
                DrawText(TextFormat("Stocks: %d", players[i]->stocks), 400, 200 + i*80, 30, WHITE);
                DrawText(TextFormat("Damage: %.0f%%", players[i]->damagePercent), 600, 200 + i*80, 30, WHITE);
            }

            DrawText("Press ENTER to return to title screen", SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT - 100, 24, WHITE);
            break;
    }

    // Draw debug info if enabled
    if (debugMode) {
        // Draw blast zones
        DrawRectangleLinesEx(
            {BLAST_ZONE_LEFT, BLAST_ZONE_TOP,
            BLAST_ZONE_RIGHT - BLAST_ZONE_LEFT,
            BLAST_ZONE_BOTTOM - BLAST_ZONE_TOP},
            2.0f,
            {255, 0, 0, 128}
        );

        // Draw player positions and states
        for (int i = 0; i < players.size(); i++) {
            Character* player = players[i];

            // Position
            DrawText(
                TextFormat("P%d Pos: (%.1f, %.1f)", i+1, player->position.x, player->position.y),
                10, SCREEN_HEIGHT - 120 + i * 20,
                16,
                WHITE
            );

            // Velocity
            DrawText(
                TextFormat("P%d Vel: (%.1f, %.1f)", i+1, player->velocity.x, player->velocity.y),
                220, SCREEN_HEIGHT - 120 + i * 20,
                16,
                WHITE
            );

            // State
            const char* stateNames[] = {
                "IDLE", "RUNNING", "JUMPING", "FALLING", "ATTACKING",
                "SHIELDING", "DODGING", "HITSTUN", "DYING"
            };

            DrawText(
                TextFormat("P%d State: %s", i+1, stateNames[player->state]),
                430, SCREEN_HEIGHT - 120 + i * 20,
                16,
                WHITE
            );

            // AI state if applicable
            if (i == 1) {
                const char* aiStateNames[] = {
                    "APPROACH", "ATTACK", "DEFEND", "RECOVER", "RETREAT", "EDGE_GUARD"
                };

                DrawText(
                    TextFormat("AI State: %s", aiStateNames[aiState.currentState]),
                    630, SCREEN_HEIGHT - 120 + i * 20,
                    16,
                    YELLOW
                );
            }
        }

        // FPS info
        DrawText(
            TextFormat("FPS: %d | Particles: %d", GetFPS(), (int)particles.size()),
            10, SCREEN_HEIGHT - 40,
            16,
            WHITE
        );
    }
}

void CleanupGame() {
    // Free allocated memory
    for (auto& player : players) {
        delete player;
    }

    players.clear();
    platforms.clear();
    particles.clear();
}

// AI IMPLEMENTATION FUNCTIONS

// Helper function to check if a position is off the main stage
bool IsOffStage(Vector2 position) {
    // Check if position is not above main platform
    bool aboveMainStage = (position.x >= SCREEN_WIDTH/2 - 300 &&
                          position.x <= SCREEN_WIDTH/2 + 300 &&
                          position.y < SCREEN_HEIGHT - 100);

    // Check if position is beyond blastzones with a margin
    bool nearBlastzone = (position.x < BLAST_ZONE_LEFT + 100 ||
                          position.x > BLAST_ZONE_RIGHT - 100 ||
                          position.y < BLAST_ZONE_TOP + 100 ||
                          position.y > BLAST_ZONE_BOTTOM - 100);

    return !aboveMainStage || nearBlastzone;
}

// Calculate threat level from player
void UpdateThreatLevel(Character* player, float absDistanceX, float absDistanceY) {
    // Base threat level depends on distance
    float distanceThreat = 1.0f - (std::min(absDistanceX, 500.0f) / 500.0f);

    // Increase threat if player is attacking
    float attackThreat = player->isAttacking ? 0.5f : 0.0f;

    // Increase threat based on player damage percent (damaged player is more desperate)
    float damageThreat = std::min(player->damagePercent / 150.0f, 0.5f);

    // Combine threat factors
    aiState.threatLevel = distanceThreat + attackThreat + damageThreat;

    // Cap threat level
    aiState.threatLevel = std::min(aiState.threatLevel, 1.0f);
}

// Determine the best AI state based on current situation
void DetermineAIState(float absDistanceX, float absDistanceY) {
    // Don't change state too frequently unless necessary
    if (aiState.stateTimer < aiState.decisionDelay &&
        aiState.currentState != AIState::RECOVER) {
        return;
    }

    AIState::State newState = aiState.currentState;

    // Priority 1: Recovery takes precedence if off stage
    if (aiState.isOffStage) {
        newState = AIState::RECOVER;
    }
    // Priority 2: Edge guarding if player is off stage and AI is safe
    else if (aiState.playerIsOffStage && !aiState.isOffStage) {
        newState = AIState::EDGE_GUARD;
    }
    // Priority 3: Defense if under threat
    else if (players[0]->isAttacking && absDistanceX < 120 &&
             absDistanceY < 100 && !aiState.wasPlayerAttacking) {
        // Player just started attacking and is close, react with defense
        newState = AIState::DEFEND;
    }
    // Priority 4: Attack if in range
    else if (absDistanceX < 80 && absDistanceY < 60) {
        newState = AIState::ATTACK;
    }
    // Priority 5: Retreat if at high damage
    else if (players[1]->damagePercent > 100 && aiState.threatLevel > 0.7f) {
        newState = AIState::RETREAT;
    }
    // Default: Approach
    else {
        newState = AIState::APPROACH;
    }

    // If state changed, reset the timer
    if (newState != aiState.currentState) {
        aiState.currentState = newState;
        aiState.stateTimer = 0;
    }
}

// Execute approach behavior - moving toward player intelligently
void ExecuteApproachBehavior(float distanceX, float distanceY, float absDistanceX, float absDistanceY) {
    // Movement toward player with advanced positioning
    if (distanceX > 20) {
        players[1]->moveRight();
        players[1]->isFacingRight = true;
    } else if (distanceX < -20) {
        players[1]->moveLeft();
        players[1]->isFacingRight = false;
    }

    // Jump to platforms or toward player with intelligent timing
    // - Jump if player is above and not directly overhead (avoid getting hit)
    // - Jump to platforms if they help approach
    if (distanceY < -50 && absDistanceX > 30 &&
        GetRandomValue(0, 100) > 50 && players[1]->state != Character::JUMPING) {
        players[1]->jump();
    }

    // Short hop aerials approach (common in competitive play)
    if (absDistanceX < 200 && absDistanceY < 100 &&
        GetRandomValue(0, 100) > 70 && players[1]->state != Character::JUMPING) {
        players[1]->jump();
        // Will execute an aerial attack in the next frame
    }

    // Fast fall when above player to pressure
    if (distanceY > 50 && players[1]->velocity.y > 0) {
        players[1]->fastFall();
    }

    // Occasionally throw out a projectile to control space
    if (absDistanceX > 200 && absDistanceX < 400 &&
        aiState.stateTimer % 60 == 0 && GetRandomValue(0, 100) > 60) {
        players[1]->neutralSpecial();
    }
}

// Execute attack behavior with optimal combos and mixups
void ExecuteAttackBehavior(float distanceX, float distanceY, float absDistanceX, float absDistanceY) {
    Character* enemy = players[1];
    Character* player = players[0];

    // Face the player
    enemy->isFacingRight = (distanceX > 0);

    // Track time since last attack to prevent button mashing
    bool canAttackNow = (aiState.stateTimer - aiState.lastAttackFrame > 10);

    if (!canAttackNow) return;

    // Ground attacks
    if (enemy->state != Character::JUMPING && enemy->state != Character::FALLING) {

        // Close range: use grab or quick attacks
        if (absDistanceX < 50 && absDistanceY < 40) {
            int attackChoice = GetRandomValue(0, 100);

            // Grab at low percents for throw combos
            if (attackChoice < 30 && player->damagePercent < 60) {
                enemy->grab();
                aiState.lastAttackFrame = aiState.stateTimer;
            }
            // Jab at very close range
            else if (attackChoice < 60) {
                enemy->jab();
                aiState.lastAttackFrame = aiState.stateTimer;
            }
            // Tilt attacks for more damage/knockback
            else if (attackChoice < 80) {
                if (distanceY < -20) {
                    enemy->upTilt(); // If player is above
                } else if (distanceY > 20) {
                    enemy->downTilt(); // If player is below
                } else {
                    enemy->forwardTilt(); // If player is beside
                }
                aiState.lastAttackFrame = aiState.stateTimer;
            }
            // Occasionally go for a smash attack for kill
            else if (player->damagePercent > 80) {
                if (distanceY < -20) {
                    enemy->upSmash(GetRandomValue(15, 30));
                } else {
                    enemy->forwardSmash(GetRandomValue(15, 30));
                }
                aiState.lastAttackFrame = aiState.stateTimer;
            }
        }
        // Medium range: use dash attack or special
        else if (absDistanceX < 150 && absDistanceY < 60) {
            int attackChoice = GetRandomValue(0, 100);

            if (attackChoice < 40) {
                // Dash in with attack
                if (distanceX > 0) {
                    enemy->moveRight();
                } else {
                    enemy->moveLeft();
                }
                enemy->dashAttack();
                aiState.lastAttackFrame = aiState.stateTimer;
            }
            else if (attackChoice < 70) {
                enemy->sideSpecial();
                aiState.lastAttackFrame = aiState.stateTimer;
            }
            else {
                // Charge smash when player is at kill percent
                if (player->damagePercent > 100) {
                    enemy->forwardSmash(GetRandomValue(20, 40));
                    aiState.lastAttackFrame = aiState.stateTimer;
                }
            }
        }
        // Long range: use projectiles or approach
        else {
            if (GetRandomValue(0, 100) > 70) {
                enemy->neutralSpecial();
                aiState.lastAttackFrame = aiState.stateTimer;
            }
        }
    }
    // Air attacks
    else {
        // Above player: use down air
        if (distanceY > 20) {
            enemy->downAir();
            aiState.lastAttackFrame = aiState.stateTimer;
        }
        // Below player: use up air
        else if (distanceY < -20) {
            enemy->upAir();
            aiState.lastAttackFrame = aiState.stateTimer;
        }
        // Beside player: use forward or back air
        else {
            if ((distanceX > 0 && enemy->isFacingRight) ||
                (distanceX < 0 && !enemy->isFacingRight)) {
                enemy->forwardAir();
            } else {
                enemy->backAir();
            }
            aiState.lastAttackFrame = aiState.stateTimer;
        }
    }

    // Handle throws if grabbing
    if (enemy->isGrabbing) {
        int throwChoice = GetRandomValue(0, 100);

        // Choose optimal throw based on position and damage
        if (player->damagePercent > 100 && enemy->position.x < 200) {
            // Back throw for KO when near edge
            enemy->backThrow();
        } else if (player->damagePercent > 100 && enemy->position.x > SCREEN_WIDTH - 200) {
            // Forward throw for KO when near edge
            enemy->forwardThrow();
        } else if (player->damagePercent < 50) {
            // Down throw for combos at low percent
            enemy->downThrow();
        } else {
            // Up throw at mid percent
            enemy->upThrow();
        }
    }
}

// Main AI update function to be called from Game.cpp UpdateGame()
void UpdateEnemyAI() {
    // Skip AI update if the player is dead or dying
    if (players[1]->stocks <= 0 || players[1]->isDying) return;

    // Skip AI update if in hitstun
    if (players[1]->isHitstun && players[1]->hitstunFrames > 5) {
        // While in hitstun, AI can still try DI (directional influence)
        // Expert AI applies optimal DI based on knockback direction
        if (players[1]->velocity.x > 0) {
            // Being knocked right, DI upward to survive longer
            players[1]->velocity.y -= 0.2f;
        } else if (players[1]->velocity.x < 0) {
            // Being knocked left, DI upward to survive longer
            players[1]->velocity.y -= 0.2f;
        }
        return;
    }

    // Get positions and calculate distances
    Vector2 playerPos = players[0]->position;
    Vector2 enemyPos = players[1]->position;
    float distanceX = playerPos.x - enemyPos.x;
    float distanceY = playerPos.y - enemyPos.y;
    float absDistanceX = fabs(distanceX);
    float absDistanceY = fabs(distanceY);

    // Update AI state timer
    aiState.stateTimer++;
    aiState.adaptiveTimer++;

    // Check if player or AI is off stage
    aiState.isOffStage = IsOffStage(enemyPos);
    aiState.playerIsOffStage = IsOffStage(playerPos);

    // Calculate threat level based on player's state and distance
    UpdateThreatLevel(players[0], absDistanceX, absDistanceY);

    // Observe player's movement patterns and adapt
    if (aiState.adaptiveTimer >= 60) {
        aiState.adaptiveTimer = 0;
        // Check if player is moving in a pattern
        float deltaX = distanceX - aiState.lastDistanceX;
        float deltaY = distanceY - aiState.lastDistanceY;

        // Store current distances for next comparison
        aiState.lastDistanceX = distanceX;
        aiState.lastDistanceY = distanceY;

        // Expert AI can predict player's movement direction
        aiState.targetPosition.x = playerPos.x + (deltaX * 10);
        aiState.targetPosition.y = playerPos.y + (deltaY * 5);
    }

    // State transitions - determine the best AI state
    DetermineAIState(absDistanceX, absDistanceY);

    // Execute behavior based on current state
    switch (aiState.currentState) {
        case AIState::APPROACH:
            ExecuteApproachBehavior(distanceX, distanceY, absDistanceX, absDistanceY);
            break;

        case AIState::ATTACK:
            ExecuteAttackBehavior(distanceX, distanceY, absDistanceX, absDistanceY);
            break;

        case AIState::DEFEND:
            ExecuteDefendBehavior(distanceX, distanceY);
            break;

        case AIState::RECOVER:
            ExecuteRecoverBehavior(distanceX, absDistanceX);
            break;

        case AIState::RETREAT:
            ExecuteRetreatBehavior(distanceX);
            break;

        case AIState::EDGE_GUARD:
            ExecuteEdgeGuardBehavior(playerPos, enemyPos);
            break;
    }

    // Track if player was attacking for reaction purposes
    aiState.wasPlayerAttacking = players[0]->isAttacking;
}// Expert edge guarding behavior to prevent player recovery
void ExecuteEdgeGuardBehavior(Vector2 playerPos, Vector2 enemyPos) {
    Character* enemy = players[1];
    Character* player = players[0];

    float distanceX = playerPos.x - enemyPos.x;
    float distanceY = playerPos.y - enemyPos.y;
    float absDistanceX = fabs(distanceX);
    float absDistanceY = fabs(distanceY);

    // Position near the edge the player is trying to recover to
    float edgeX = (playerPos.x < SCREEN_WIDTH/2) ?
                  SCREEN_WIDTH/2 - 300 :
                  SCREEN_WIDTH/2 + 300;

    // Move toward the edge
    if (enemyPos.x < edgeX - 50) {
        enemy->moveRight();
    } else if (enemyPos.x > edgeX + 50) {
        enemy->moveLeft();
    }

    // Choose edge guarding strategy based on player's position

    // Player is trying to recover from below
    if (playerPos.y > SCREEN_HEIGHT - 150) {
        // If player is far below, prepare for their recovery
        if (playerPos.y > SCREEN_HEIGHT) {
            // Wait at edge
            if (abs(enemyPos.x - edgeX) < 50) {
                // Occasionally charge a smash attack
                if (aiState.stateTimer % 60 == 0 && GetRandomValue(0, 100) > 70) {
                    enemy->downSmash(GetRandomValue(10, 30));
                }

                // Or prepare to intercept with an aerial
                if (aiState.stateTimer % 45 == 0 && GetRandomValue(0, 100) > 80) {
                    // Jump off stage to intercept
                    enemy->jump();
                }
            }
        }
        // Player is close enough to intercept
        else if (absDistanceX < 150 && absDistanceY < 150) {
            // Jump off stage for aggressive edge guard
            if (enemy->state != Character::JUMPING &&
                enemy->state != Character::FALLING &&
                GetRandomValue(0, 100) > 40) {
                enemy->jump();
            }

            // Use appropriate aerial based on position
            if (enemy->state == Character::JUMPING ||
                enemy->state == Character::FALLING) {

                if (distanceY > 0 && absDistanceX < 100) {
                    // Player is below, use down air (spike)
                    enemy->downAir();
                } else if (absDistanceY < 50) {
                    // Player is beside, use back/forward air
                    if ((distanceX > 0 && enemy->isFacingRight) ||
                        (distanceX < 0 && !enemy->isFacingRight)) {
                        enemy->forwardAir();
                    } else {
                        enemy->backAir();
                    }
                }
            }
        }
    }
    // Player is trying to recover from the side
    else if ((playerPos.x < SCREEN_WIDTH/2 - 300 ||
              playerPos.x > SCREEN_WIDTH/2 + 300) &&
              playerPos.y < SCREEN_HEIGHT - 100) {

        // If player is attempting side recovery
        if (abs(playerPos.y - enemyPos.y) < 100) {
            // Use projectiles or side special to intercept
            if (aiState.stateTimer % 30 == 0 && GetRandomValue(0, 100) > 60) {
                enemy->neutralSpecial();
            } else if (aiState.stateTimer % 45 == 0 && GetRandomValue(0, 100) > 70) {
                enemy->sideSpecial();
            }
        }
        // Jump and intercept with aerial
        else if (playerPos.y < enemyPos.y &&
                aiState.stateTimer % 40 == 0 &&
                GetRandomValue(0, 100) > 50) {
            enemy->jump();

            // Wait to use aerial at right time
            if (enemy->state == Character::JUMPING &&
                abs(playerPos.y - enemyPos.y) < 50) {
                if ((distanceX > 0 && enemy->isFacingRight) ||
                    (distanceX < 0 && !enemy->isFacingRight)) {
                    enemy->forwardAir();
                } else {
                    enemy->backAir();
                }
            }
        }
    }

    // Return to stage if AI is getting too risky with edge guard
    if (IsOffStage(enemyPos) &&
        (enemyPos.y > SCREEN_HEIGHT - 100 ||
         enemyPos.x < BLAST_ZONE_LEFT + 150 ||
         enemyPos.x > BLAST_ZONE_RIGHT - 150)) {
        aiState.currentState = AIState::RECOVER;
    }

    // If player made it back to stage, return to approach
    if (!aiState.playerIsOffStage) {
        aiState.currentState = AIState::APPROACH;
    }
}// Expert retreat behavior to reset neutral and avoid damage
void ExecuteRetreatBehavior(float distanceX) {
    Character* enemy = players[1];

    // Move away from player
    if (distanceX > 0) {
        enemy->moveLeft();
        enemy->isFacingRight = true; // Still face player while retreating
    } else {
        enemy->moveRight();
        enemy->isFacingRight = false; // Still face player while retreating
    }

    // Shield if player approaches too quickly
    if (abs(distanceX) < 100 && players[0]->velocity.x != 0 &&
        ((distanceX > 0 && players[0]->velocity.x > 3) ||
         (distanceX < 0 && players[0]->velocity.x < -3))) {
        enemy->shield();
    }

    // Jump to platform to reset
    if (aiState.stateTimer % 30 == 0 && GetRandomValue(0, 100) > 70) {
        enemy->jump();
    }

    // Use projectiles to keep player away
    if (aiState.stateTimer % 45 == 0 && GetRandomValue(0, 100) > 60) {
        enemy->neutralSpecial();
    }

    // Check if we've retreated enough
    if (abs(distanceX) > 300 || aiState.stateTimer > 90) {
        aiState.currentState = AIState::APPROACH;
        aiState.stateTimer = 0;
    }
}// Expert recovery behavior to get back to stage
void ExecuteRecoverBehavior(float distanceX, float absDistanceX) {
    Character* enemy = players[1];

    // First priority: get horizontal alignment with stage
    if (enemy->position.x < SCREEN_WIDTH/2 - 300) {
        enemy->moveRight();
    } else if (enemy->position.x > SCREEN_WIDTH/2 + 300) {
        enemy->moveLeft();
    }

    // Use resources intelligently for recovery

    // If below stage and has double jump, use it when close enough
    if (enemy->position.y > SCREEN_HEIGHT - 150 && absDistanceX < 350 &&
        enemy->hasDoubleJump && !enemy->isJumping) {
        enemy->jump(); // This will use double jump if needed
    }

    // Use up special for recovery, but save it for the right moment
    if (enemy->position.y > SCREEN_HEIGHT - 100 &&
        absDistanceX < 300 && !enemy->isJumping && !enemy->hasDoubleJump &&
        enemy->specialUpCD.current <= 0) {
        enemy->upSpecial();
    }

    // Air dodge as a last resort if it can help recovery
    if (enemy->position.y > SCREEN_HEIGHT - 150 &&
        absDistanceX < 250 && !enemy->isJumping && !enemy->hasDoubleJump &&
        enemy->specialUpCD.current > 0 && !enemy->isDodging &&
        GetRandomValue(0, 100) > 70) {

        // Calculate best air dodge angle for recovery
        float dodgeX = (enemy->position.x < SCREEN_WIDTH/2) ? 1.0f : -1.0f;
        float dodgeY = -1.0f;
        enemy->airDodge(dodgeX, dodgeY);
    }

    // If close to danger zone, prioritize getting back
    if (enemy->position.y > BLAST_ZONE_BOTTOM - 150) {
        // Maximum effort to recover - mash jump and up special
        if (!enemy->isJumping && enemy->hasDoubleJump) {
            enemy->jump();
        } else if (enemy->specialUpCD.current <= 0) {
            enemy->upSpecial();
        }
    }
}// Execute defensive options with optimal timing
void ExecuteDefendBehavior(float distanceX, float distanceY) {
    Character* enemy = players[1];
    Character* player = players[0];

    // Face the player
    enemy->isFacingRight = (distanceX > 0);

    // If player is attacking, choose best defensive option
    if (player->isAttacking) {
        int defenseChoice = GetRandomValue(0, 100);

        // Perfect shielding (precise timing)
        if (defenseChoice < 40 && abs(distanceX) < 80 && abs(distanceY) < 60) {
            enemy->shield();
        }
        // Spotdodge (for close attacks, especially grabs)
        else if (defenseChoice < 60 && abs(distanceX) < 50 && abs(distanceY) < 40) {
            enemy->spotDodge();
        }
        // Roll away from danger
        else if (defenseChoice < 85) {
            if (distanceX > 0) {
                enemy->backDodge(); // Roll away if player is to the right
            } else {
                enemy->forwardDodge(); // Roll away if player is to the left
            }
        }
        // Jump away (especially good for avoiding ground attacks)
        else if (enemy->state != Character::JUMPING && enemy->state != Character::FALLING) {
            enemy->jump();

            // Air dodge if needed after jump
            if (GetRandomValue(0, 100) > 70) {
                float dodgeX = (distanceX > 0) ? -1.0f : 1.0f;
                float dodgeY = -0.5f;
                enemy->airDodge(dodgeX, dodgeY);
            }
        }
    }
    // If player is grabbing or has grabbed, escape
    else if (player->isGrabbing) {
        // Button mashing to escape grabs faster
        int escapeAction = GetRandomValue(0, 3);
        switch (escapeAction) {
            case 0: enemy->moveLeft(); break;
            case 1: enemy->moveRight(); break;
            case 2: enemy->jump(); break;
            case 3: enemy->shield(); enemy->releaseShield(); break;
        }
    }
    // If shielding and player isn't attacking anymore, release shield
    else if (enemy->isShielding && !player->isAttacking) {
        enemy->releaseShield();

        // Optional grab after shield for shield grab punish
        if (abs(distanceX) < 60 && abs(distanceY) < 40 && GetRandomValue(0, 100) > 50) {
            enemy->grab();
        }
    }
    // If state has lasted too long, return to approaching
    else if (aiState.stateTimer > 60) {
        aiState.currentState = AIState::APPROACH;
        aiState.stateTimer = 0;
    }
}