#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "raylib.h"
#include "Character.h"
#include "Platform.h"
#include "Particle.h"
#include "GameConfig.h"
#include <vector>
#include <string>

// Forward declarations
class Item;
class Stage;

// Game state manager for Smash Bros style game
class GameState {
public:
    enum State {
        TITLE_SCREEN,
        CHARACTER_SELECT,
        STAGE_SELECT,
        GAME_START,
        GAME_PLAYING,
        GAME_PAUSED,
        GAME_SUDDEN_DEATH,
        GAME_OVER,
        RESULTS_SCREEN
    };

    // Match settings
    struct MatchSettings {
        int stockCount;
        int timeLimit;        // In seconds, 0 for infinite
        bool itemsEnabled;
        float itemFrequency;
        bool stageHazards;
        bool finalSmash;      // Whether final smash meter/ball is enabled
    };

    // Current state
    State currentState;
    int stateTimer;
    bool isPaused;

    // Match configuration
    MatchSettings settings;
    int currentTime;          // Time remaining in seconds
    bool isStockMatch;        // Stock match vs. Time match
    bool isSuddenDeath;

    // Players and stage
    std::vector<Character*> players;
    std::vector<Platform> platforms;
    std::vector<Vector2> spawnPoints;
    std::vector<Particle> particles;
    std::vector<Item*> items;

    // Stage boundaries and blastzones
    Rectangle stageBounds;
    Rectangle blastZones;

    // UI elements
    Texture2D stockIcons[4];   // Stock icons for each player

    // Results
    struct PlayerResult {
        std::string name;
        int stocksRemaining;
        int falls;            // How many times they fell off
        int KOs;              // How many knockouts they scored
        int damageDealt;
        int damageTaken;
        int selfDestructs;
    };

    std::vector<PlayerResult> results;
    int winnerIndex;

    // Constructor
    GameState();

    // Core methods
    void initialize();
    void update();
    void draw();

    // State transition methods
    void changeState(State newState);
    void startMatch(const MatchSettings& settings);
    void endMatch();
    void pauseGame();
    void resumeGame();
    void resetMatch();

    // Match management
    void checkMatchEnd();
    void startSuddenDeath();
    void processResults();
    void respawnPlayer(int playerIndex);

    // Item management
    void spawnRandomItem();
    void updateItems();
    void drawItems();

    // Draw methods for different states
    void drawTitleScreen();
    void drawCharacterSelect();
    void drawStageSelect();
    void drawHUD();
    void drawGamePlaying();
    void drawGamePaused();
    void drawGameOver();
    void drawResultsScreen();

    // Helper methods
    bool isMatchTimeUp();
    bool checkAllPlayersDead();
    int getLeadingPlayer();
    int getRemainingPlayers();

    // Debug functions
    void toggleDebugMode();
    void drawDebugInfo();

private:
    bool debugMode;
    std::string debugText;
    int titleOptionSelected;
    int characterSelectIndex[4];
    int stageSelectIndex;
};

#endif // GAMESTATE_H