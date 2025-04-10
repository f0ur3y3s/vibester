#include "raylib.h"
#include "Character.h"
#include "Platform.h"
#include "Particle.h"
#include "ParticleSystem.h"
#include "GameConfig.h"
#include "Item.h"
#include "GameState.h"
#include "EnhancedAIController.h" // Updated include for the new AI architecture
#include "StateManager.h" // Include state definitions
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>

// Main game functions
void InitGame();
void UpdateGame();
void DrawGame();
void CleanupGame();

// Use enums directly
using CharacterState::IDLE;
using CharacterState::RUNNING;
using CharacterState::JUMPING;
using CharacterState::FALLING;
using CharacterState::ATTACKING;
using CharacterState::SHIELDING;
using CharacterState::DODGING;
using CharacterState::HITSTUN;
using CharacterState::DYING;

// Global game variables
GameState gameState;
std::vector<Character*> players;
std::vector<Platform> platforms;
std::vector<Vector2> spawnPoints;
std::vector<Particle> particles;
Font gameFont;
bool debugMode = false;

// Create an instance of the enhanced AI controller
std::unique_ptr<EnhancedAIController> enhancedAI;
float difficultyLevel = 0.8f; // Default to challenging (0.0 to 1.0)

// Main entry point
int main()
{
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Super Smash Clone - Advanced AI Mode");
    SetTargetFPS(60);

    // Initialize game
    InitGame();

    // Main game loop
    while (!WindowShouldClose())
    {
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

void InitGame()
{
    // Load font
    gameFont = GetFontDefault();

    // Create platforms with appropriate types
    // Main/bottom platform (SOLID - has full collision)
    platforms.push_back(Platform(
        SCREEN_WIDTH / 2 - 300, SCREEN_HEIGHT - 100,
        600, 50,
        DARKGRAY,
        SOLID // Main platform is solid
    ));

    // Side platforms (PASSTHROUGH - only collide from top)
    platforms.push_back(Platform(
        SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT - 250,
        150, 20,
        GRAY,
        PASSTHROUGH // Side platforms are passthrough
    ));

    platforms.push_back(Platform(
        SCREEN_WIDTH / 2 + 100, SCREEN_HEIGHT - 250,
        150, 20,
        GRAY,
        PASSTHROUGH // Side platforms are passthrough
    ));

    // Top platform (PASSTHROUGH)
    platforms.push_back(Platform(
        SCREEN_WIDTH / 2 - 75, SCREEN_HEIGHT - 400,
        150, 20,
        GRAY,
        PASSTHROUGH // Top platform is passthrough
    ));

    // Create spawn points
    spawnPoints.push_back({SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 200});
    spawnPoints.push_back({SCREEN_WIDTH / 2 + 200, SCREEN_HEIGHT - 200});
    spawnPoints.push_back({SCREEN_WIDTH / 2, SCREEN_HEIGHT - 200});
    spawnPoints.push_back({SCREEN_WIDTH / 2, SCREEN_HEIGHT - 300});

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

    // Initialize the AI controller
    enhancedAI = std::make_unique<EnhancedAIController>();
    enhancedAI->SetDifficulty(difficultyLevel);

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

void UpdateGame()
{
    // Process game state
    switch (gameState.currentState)
    {
    case GameState::TITLE_SCREEN:
        // Title screen logic
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
        {
            gameState.changeState(GameState::GAME_START);
        }

    // Difficulty setting with clear indication
        if (IsKeyPressed(KEY_ONE))
        {
            difficultyLevel = 0.2f; // Easy
            enhancedAI->SetDifficulty(difficultyLevel);
            DrawText("EASY MODE SELECTED", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 20, GREEN);
        }
        else if (IsKeyPressed(KEY_TWO))
        {
            difficultyLevel = 0.5f; // Medium
            enhancedAI->SetDifficulty(difficultyLevel);
            DrawText("MEDIUM MODE SELECTED", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 20, YELLOW);
        }
        else if (IsKeyPressed(KEY_THREE))
        {
            difficultyLevel = 0.8f; // Hard
            enhancedAI->SetDifficulty(difficultyLevel);
            DrawText("HARD MODE SELECTED", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 20, ORANGE);
        }
        else if (IsKeyPressed(KEY_FOUR))
        {
            difficultyLevel = 1.0f; // Expert
            enhancedAI->SetDifficulty(difficultyLevel);
            DrawText("EXPERT MODE SELECTED", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 50, 20, RED);
        }
        break;

    case GameState::CHARACTER_SELECT:
        // Character selection logic
        if (IsKeyPressed(KEY_ENTER))
        {
            gameState.changeState(GameState::STAGE_SELECT);
        }
        break;

    case GameState::STAGE_SELECT:
        // Stage selection logic
        if (IsKeyPressed(KEY_ENTER))
        {
            gameState.changeState(GameState::GAME_START);
        }
        break;

    case GameState::GAME_START:
        // Game start countdown
        gameState.stateTimer++;
        if (gameState.stateTimer >= GAME_START_TIMER)
        {
            gameState.changeState(GameState::GAME_PLAYING);
        }
        break;

    case GameState::GAME_PLAYING:
        // Check for pause
        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
        {
            gameState.pauseGame();
            break;
        }

    // Update debug mode
        if (IsKeyPressed(KEY_F1))
        {
            debugMode = !debugMode;
        }

    // Update players
        for (auto& player : players)
        {
            player->update(platforms);
        }

    // Check for character collisions for attacks
        for (auto& attacker : players)
        {
            // Skip players who are dying or exploding
            if (attacker->stateManager.isDying || attacker->stateManager.isExploding) {
                continue;
            }
            
            if (attacker->stateManager.isAttacking)
            {
                for (auto& defender : players)
                {
                    if (attacker != defender)
                    {
                        attacker->checkHit(*defender);
                    }
                }
            }
        }

    // Update particles
        for (int i = 0; i < particles.size(); i++)
        {
            if (!particles[i].update())
            {
                particles.erase(particles.begin() + i);
                i--;
            }
        }

    // Player 1 controls
        if (players[0]->stocks > 0 && !players[0]->stateManager.isDying)
        {
            // Movement
            if (IsKeyDown(KEY_A)) players[0]->moveLeft();
            if (IsKeyDown(KEY_D)) players[0]->moveRight();
            if (IsKeyPressed(KEY_W)) players[0]->jump();

            // Fast fall / platform drop-through
            if (IsKeyDown(KEY_S))
            {
                if (players[0]->stateManager.state == IDLE || players[0]->stateManager.state == RUNNING)
                {
                    // On ground, attempt to drop through platform
                    players[0]->dropThroughPlatform();
                }
                else if (players[0]->stateManager.state == FALLING)
                {
                    // In air, fast fall
                    players[0]->fastFall();
                }
            }

            // Attacks
            if (IsKeyPressed(KEY_J))
            {
                // Basic attack - context sensitive
                if (players[0]->stateManager.state == JUMPING || players[0]->stateManager.state == FALLING)
                {
                    players[0]->neutralAir();
                }
                else
                {
                    players[0]->jab();
                }
            }

            if (IsKeyPressed(KEY_K))
            {
                // Special attack - context sensitive
                if (IsKeyDown(KEY_A))
                {
                    // Side special left
                    players[0]->sideSpecial();
                }
                else if (IsKeyDown(KEY_D))
                {
                    // Side special right
                    players[0]->sideSpecial();
                }
                else if (IsKeyDown(KEY_W))
                {
                    // Up special
                    players[0]->upSpecial();
                }
                else if (IsKeyDown(KEY_S))
                {
                    // Down special
                    players[0]->downSpecial();
                }
                else
                {
                    // Neutral special
                    players[0]->neutralSpecial();
                }
            }

            // Smash attacks
            if (IsKeyDown(KEY_L))
            {
                if (IsKeyDown(KEY_A))
                {
                    // Forward smash left
                    players[0]->forwardSmash(20.0f);
                }
                else if (IsKeyDown(KEY_D))
                {
                    // Forward smash right
                    players[0]->forwardSmash(20.0f);
                }
                else if (IsKeyDown(KEY_W))
                {
                    // Up smash
                    players[0]->upSmash(20.0f);
                }
                else if (IsKeyDown(KEY_S))
                {
                    // Down smash
                    players[0]->downSmash(20.0f);
                }
            }

            // Shield/Dodge
            if (IsKeyDown(KEY_I))
            {
                if (IsKeyPressed(KEY_A))
                {
                    players[0]->forwardDodge();
                }
                else if (IsKeyPressed(KEY_D))
                {
                    players[0]->backDodge();
                }
                else if (IsKeyPressed(KEY_S))
                {
                    players[0]->spotDodge();
                }
                else
                {
                    players[0]->shield();
                }
            }
            else if (IsKeyReleased(KEY_I))
            {
                players[0]->releaseShield();
            }

            // Grab
            if (IsKeyPressed(KEY_U))
            {
                players[0]->grab();
            }

            // Throws (when grabbing)
            if (players[0]->stateManager.isGrabbing)
            {
                if (IsKeyPressed(KEY_J))
                {
                    players[0]->pummel();
                }
                else if (IsKeyPressed(KEY_A))
                {
                    players[0]->backThrow();
                }
                else if (IsKeyPressed(KEY_D))
                {
                    players[0]->forwardThrow();
                }
                else if (IsKeyPressed(KEY_W))
                {
                    players[0]->upThrow();
                }
                else if (IsKeyPressed(KEY_S))
                {
                    players[0]->downThrow();
                }
            }

            // Aerial controls - more specific aerial attacks
            if (players[0]->stateManager.state == JUMPING || players[0]->stateManager.state == FALLING)
            {
                if (IsKeyPressed(KEY_J))
                {
                    if (IsKeyDown(KEY_A))
                    {
                        players[0]->backAir();
                    }
                    else if (IsKeyDown(KEY_D))
                    {
                        players[0]->forwardAir();
                    }
                    else if (IsKeyDown(KEY_W))
                    {
                        players[0]->upAir();
                    }
                    else if (IsKeyDown(KEY_S))
                    {
                        players[0]->downAir();
                    }
                    else
                    {
                        players[0]->neutralAir();
                    }
                }
            }
        }

    // Run the enhanced AI for the enemy
        enhancedAI->Update(players, platforms);

    // Check for game end conditions
        {
            bool allButOneDead = true;
            int aliveCount = 0;

            for (auto& player : players)
            {
                if (player->stocks > 0)
                {
                    aliveCount++;
                }
            }

            if (aliveCount <= 1)
            {
                gameState.changeState(GameState::GAME_OVER);
            }
        }
        break;

    case GameState::GAME_PAUSED:
        // Pause menu logic
        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
        {
            gameState.resumeGame();
        }

        if (IsKeyPressed(KEY_R))
        {
            gameState.resetMatch();
        }
        break;

    case GameState::GAME_OVER:
        // Game over logic
        gameState.stateTimer++;
        if (gameState.stateTimer >= GAME_END_DELAY)
        {
            gameState.changeState(GameState::RESULTS_SCREEN);
        }
        break;

    case GameState::RESULTS_SCREEN:
        // Results screen logic
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
        {
            gameState.resetMatch();
            gameState.changeState(GameState::TITLE_SCREEN);
        }
        break;

    default:
        break;
    }
}

void DrawGame()
{
    // Draw background
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {135, 206, 235, 255}); // Sky blue

    // Draw platforms
    for (auto& platform : platforms)
    {
        platform.draw();
    }

    // Draw particles
    for (auto& particle : particles)
    {
        particle.draw();
    }

    // Draw players
    for (auto& player : players)
    {
        player->draw();
    }

    // Draw HUD
    for (int i = 0; i < players.size(); i++)
    {
        Color playerColor = players[i]->color;

        // Stock icons
        for (int s = 0; s < players[i]->stocks; s++)
        {
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
            TextFormat("P%d: %.0f%%", i + 1, players[i]->damagePercent),
            HUD_MARGIN + i * 200,
            HUD_MARGIN + STOCK_ICON_SIZE + 5,
            DAMAGE_FONT_SIZE,
            playerColor
        );
    }

    // Draw state-specific screens
    switch (gameState.currentState)
    {
    case GameState::TITLE_SCREEN:
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});
            DrawText("SUPER SMASH CLONE - ADVANCED AI MODE", SCREEN_WIDTH / 2 - 320, SCREEN_HEIGHT / 4, 40, WHITE);
            DrawText("Press ENTER to Start", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2, 30, WHITE);
            DrawText("Player Controls: WASD to move, J to attack, K for special, L for smash",
                     SCREEN_WIDTH / 2 - 300, SCREEN_HEIGHT - 220, 20, WHITE);
            DrawText("I to shield/dodge, U to grab", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT - 190, 20, WHITE);
            DrawText("Select Difficulty:", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 150, 20, WHITE);

            // Difficulty selection
            Color easyColor = (difficultyLevel == 0.2f) ? GREEN : GRAY;
            Color mediumColor = (difficultyLevel == 0.5f) ? YELLOW : GRAY;
            Color hardColor = (difficultyLevel == 0.8f) ? ORANGE : GRAY;
            Color expertColor = (difficultyLevel == 1.0f) ? RED : GRAY;

            DrawText("1: Easy", SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 120, 20, easyColor);
            DrawText("2: Medium", SCREEN_WIDTH / 2 - 70, SCREEN_HEIGHT - 120, 20, mediumColor);
            DrawText("3: Hard", SCREEN_WIDTH / 2 + 60, SCREEN_HEIGHT - 120, 20, hardColor);
            DrawText("4: Expert", SCREEN_WIDTH / 2 + 170, SCREEN_HEIGHT - 120, 20, expertColor);

            DrawText("Can you defeat the AI? Good luck!",
                     SCREEN_WIDTH / 2 - 170, SCREEN_HEIGHT - 80, 20, WHITE);
        }
        break;

    case GameState::GAME_START:
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 100});
            int countdown = (GAME_START_TIMER - gameState.stateTimer) / 60 + 1;
            DrawText(TextFormat("%d", countdown), SCREEN_WIDTH / 2 - 20, SCREEN_HEIGHT / 2 - 50, 100, WHITE);
        }
        break;

    case GameState::GAME_PAUSED:
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});
            DrawText("PAUSED", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 3, 50, WHITE);
            DrawText("Press P to Resume", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 2, 30, WHITE);
            DrawText("Press R to Restart", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 2 + 40, 30, WHITE);
        }
        break;

    case GameState::GAME_OVER:
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});

            // Find the winner
            int winnerId = -1;
            for (int i = 0; i < players.size(); i++)
            {
                if (players[i]->stocks > 0)
                {
                    winnerId = i;
                    break;
                }
            }

            if (winnerId == 0)
            {
                DrawText("YOU WIN!", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 3, 50, GREEN);
            }
            else if (winnerId == 1)
            {
                DrawText("AI WINS!", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 3, 50, RED);
            }
            else
            {
                DrawText("DRAW!", SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 3, 40, WHITE);
            }
        }
        break;

    case GameState::RESULTS_SCREEN:
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});
            DrawText("RESULTS", SCREEN_WIDTH / 2 - 80, 100, 40, WHITE);

            for (int i = 0; i < players.size(); i++)
            {
                Color playerColor = players[i]->color;
                std::string displayName = (i == 0) ? "You" : "Enhanced AI";
                DrawText(displayName.c_str(), 200, 200 + i * 80, 30, playerColor);
                DrawText(TextFormat("Stocks: %d", players[i]->stocks), 400, 200 + i * 80, 30, WHITE);
                DrawText(TextFormat("Damage: %.0f%%", players[i]->damagePercent), 600, 200 + i * 80, 30, WHITE);
            }

            DrawText("Press ENTER to return to title screen", SCREEN_WIDTH / 2 - 220, SCREEN_HEIGHT - 100, 24, WHITE);
        }
        break;
    }

    // Draw debug info if enabled
    if (debugMode)
    {
        // Draw blast zones
        DrawRectangleLinesEx(
            {
                BLAST_ZONE_LEFT, BLAST_ZONE_TOP,
                BLAST_ZONE_RIGHT - BLAST_ZONE_LEFT,
                BLAST_ZONE_BOTTOM - BLAST_ZONE_TOP
            },
            2.0f,
            {255, 0, 0, 128}
        );

        // Draw player positions and states
        for (int i = 0; i < players.size(); i++)
        {
            Character* player = players[i];

            // Position
            DrawText(
                TextFormat("P%d Pos: (%.1f, %.1f)", i + 1, player->physics.position.x, player->physics.position.y),
                10, SCREEN_HEIGHT - 120 + i * 20,
                16,
                WHITE
            );

            // Velocity
            DrawText(
                TextFormat("P%d Vel: (%.1f, %.1f)", i + 1, player->physics.velocity.x, player->physics.velocity.y),
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
                TextFormat("P%d State: %s", i + 1, stateNames[player->stateManager.state]),
                430, SCREEN_HEIGHT - 120 + i * 20,
                16,
                WHITE
            );

            // AI state if applicable
            if (i == 1)
            {
                const char* aiStateNames[] = {
                    "NEUTRAL", "APPROACH", "ATTACK", "PRESSURE", "BAIT",
                    "DEFEND", "PUNISH", "RECOVER", "RETREAT", "EDGE_GUARD",
                    "LEDGE_TRAP", "COMBO"
                };

                // Get current AI state from EnhancedAIController
                EnhancedAIState::State currentAIState = enhancedAI->GetCurrentState();
                float confidence = enhancedAI->GetCurrentConfidence();

                DrawText(
                    TextFormat("AI State: %s (%.2f)", aiStateNames[currentAIState], confidence),
                    630, SCREEN_HEIGHT - 120 + i * 20,
                    16,
                    YELLOW
                );
            }
        }

        // FPS info and difficulty
        DrawText(
            TextFormat("FPS: %d | Particles: %d | Difficulty: %.1f",
                       GetFPS(), (int)particles.size(), difficultyLevel),
            10, SCREEN_HEIGHT - 40,
            16,
            WHITE
        );
    }
}

void CleanupGame()
{
    // Free allocated memory
    for (auto& player : players)
    {
        delete player;
    }

    players.clear();
    platforms.clear();
    particles.clear();

    // Clear AI controller
    enhancedAI.reset();
}
