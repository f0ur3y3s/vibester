#include "raylib.h"
#include "Constants.h"
#include "Character.h"
#include "Platform.h"
#include "Particle.h"
#include "ParticleSystem.h"
#include <vector>
#include <algorithm>
#include <string>

int main() {
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Toilet Brawl - Raylib Implementation");
    SetTargetFPS(60);
    
    // Create characters
    Character player(SCREEN_WIDTH / 3, 100, 60, 80, 5.0f, SKYBLUE, "The Throne");
    Character enemy(SCREEN_WIDTH * 2 / 3, 100, 60, 80, 4.0f, PINK, "The Plunger");
    
    // Create platforms
    std::vector<Platform> platforms = {
        Platform(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 100, 400, 50, DARKGREEN),
        Platform(100, SCREEN_HEIGHT - 200, 200, 20, DARKGREEN),
        Platform(SCREEN_WIDTH - 300, SCREEN_HEIGHT - 200, 200, 20, DARKGREEN),
        Platform(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 300, 200, 20, DARKGREEN)
    };
    
    // Particle system
    std::vector<Particle> particles;
    
    // Track death events for particle generation
    bool playerDiedLastFrame = false;
    bool enemyDiedLastFrame = false;
    
    // Game loop
    while (!WindowShouldClose()) {
        // Update
        
        // Player controls
        if (IsKeyDown(KEY_LEFT)) player.moveLeft();
        if (IsKeyDown(KEY_RIGHT)) player.moveRight();
        if (IsKeyPressed(KEY_UP)) player.jump();
        
        // Standard Attacks
        if (IsKeyPressed(KEY_Z)) player.neutralAttack();
        if (IsKeyPressed(KEY_X)) player.sideAttack();
        if (IsKeyPressed(KEY_C)) player.upAttack();
        if (IsKeyPressed(KEY_V)) player.downAttack();
        
        // Special Attacks
        if (IsKeyPressed(KEY_A)) player.specialNeutralAttack();
        if (IsKeyPressed(KEY_S)) player.specialSideAttack();
        if (IsKeyPressed(KEY_D)) player.specialUpAttack();
        if (IsKeyPressed(KEY_F)) player.specialDownAttack();
        
        // Simple enemy AI
        if (enemy.position.x < player.position.x - 10) enemy.moveRight();
        else if (enemy.position.x > player.position.x + 10) enemy.moveLeft();
        
        if (GetRandomValue(0, 100) < 2) enemy.jump();
        
        // Enemy AI attack selection
        int attackChoice = GetRandomValue(0, 100);
        if (attackChoice < 3) enemy.neutralAttack();
        else if (attackChoice < 5) enemy.sideAttack();
        else if (attackChoice < 7) enemy.upAttack();
        else if (attackChoice < 9) enemy.downAttack();
        
        // Special attacks are less frequent
        int specialChoice = GetRandomValue(0, 300);
        if (specialChoice < 1) enemy.specialNeutralAttack();
        else if (specialChoice < 2) enemy.specialSideAttack();
        else if (specialChoice < 3) enemy.specialUpAttack();
        else if (specialChoice < 4) enemy.specialDownAttack();
        
        // Check if either character is starting death animation
        bool playerStartingDeath = !player.isDying && player.position.y > SCREEN_HEIGHT + 90;
        bool enemyStartingDeath = !enemy.isDying && enemy.position.y > SCREEN_HEIGHT + 90;
        
        // Update characters
        player.update(platforms);
        enemy.update(platforms);
        
        // Check for newly started death animations
        if (!playerDiedLastFrame && player.isDying) {
            // Create blast particles for player death
            auto deathParticles = createBlastParticles(player.deathPosition, 40, player.color);
            particles.insert(particles.end(), deathParticles.begin(), deathParticles.end());
            
            // Play smash-like "blast off" effect
            // In a real game, you would add sound here
        }
        
        if (!enemyDiedLastFrame && enemy.isDying) {
            // Create blast particles for enemy death
            auto deathParticles = createBlastParticles(enemy.deathPosition, 40, enemy.color);
            particles.insert(particles.end(), deathParticles.begin(), deathParticles.end());
        }
        
        // Update death tracking
        playerDiedLastFrame = player.isDying;
        enemyDiedLastFrame = enemy.isDying;
        
        // Check for hits
        if (player.checkHit(enemy)) {
            // Create hit particles
            auto newParticles = createSplashParticles(enemy.position, 20);
            particles.insert(particles.end(), newParticles.begin(), newParticles.end());
            
            // If damage is very high, create more particles
            if (enemy.damage > 100) {
                auto extraParticles = createSplashParticles(enemy.position, enemy.damage / 20);
                particles.insert(particles.end(), extraParticles.begin(), extraParticles.end());
            }
        }
        
        if (enemy.checkHit(player)) {
            // Create hit particles
            auto newParticles = createSplashParticles(player.position, 20);
            particles.insert(particles.end(), newParticles.begin(), newParticles.end());
            
            // If damage is very high, create more particles
            if (player.damage > 100) {
                auto extraParticles = createSplashParticles(player.position, player.damage / 20);
                particles.insert(particles.end(), extraParticles.begin(), extraParticles.end());
            }
        }
        
        // Update particles
        particles.erase(
            std::remove_if(particles.begin(), particles.end(), [](Particle& p) { 
                return !p.update(); 
            }),
            particles.end()
        );
        
        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw bathroom background
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{230, 240, 255, 255});
        DrawRectangle(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50, DARKBLUE);
        
        // Draw some bathroom tiles
        for (int x = 0; x < SCREEN_WIDTH; x += 40) {
            for (int y = 0; y < SCREEN_HEIGHT - 50; y += 40) {
                DrawRectangleLines(x, y, 40, 40, Color{200, 220, 255, 100});
            }
        }
        
        // Draw platforms
        for (auto& platform : platforms) {
            platform.draw();
        }
        
        // Draw particles
        for (auto& particle : particles) {
            particle.draw();
        }
        
        // Draw characters (draw dying characters first so they appear behind)
        if (player.isDying) player.draw();
        if (enemy.isDying) enemy.draw();
        
        // Draw active characters on top
        if (!player.isDying) player.draw();
        if (!enemy.isDying) enemy.draw();
        
        // Draw UI
        DrawText("TOILET BRAWL", 20, 20, 30, DARKBLUE);
        
        // Show blast-off message when a character dies
        if (player.isDying) {
            DrawText("PLAYER BLASTED OFF!", SCREEN_WIDTH/2 - 150, 200, 30, RED);
        }
        if (enemy.isDying) {
            DrawText("ENEMY BLASTED OFF!", SCREEN_WIDTH/2 - 150, 250, 30, RED);
        }
        
        // Draw controls
        DrawText("Movement: Arrow Keys", 20, 60, 18, DARKGRAY);
        DrawText("Standard Attacks: Z (Neutral), X (Side), C (Up), V (Down)", 20, 85, 18, DARKGRAY);
        DrawText("Special Attacks: A (Water Cannon), S (Rolling Flush)", 20, 110, 18, DARKGRAY);
        DrawText("                D (Geyser Recovery), F (Swirl Counter)", 20, 135, 18, DARKGRAY);
        
        // Draw damage meters
        DrawText("Player Damage:", 20, SCREEN_HEIGHT - 40, 20, DARKBLUE);
        DrawRectangle(170, SCREEN_HEIGHT - 35, player.damage * 2, 20, RED);
        
        DrawText("Enemy Damage:", SCREEN_WIDTH - 250, SCREEN_HEIGHT - 40, 20, DARKBLUE);
        DrawRectangle(SCREEN_WIDTH - 100, SCREEN_HEIGHT - 35, enemy.damage * 2, 20, RED);
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}