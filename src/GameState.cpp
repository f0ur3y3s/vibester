#include "GameState.h"
#include "ParticleSystem.h"
#include <algorithm>

// Constructor
GameState::GameState() {
    currentState = TITLE_SCREEN;
    stateTimer = 0;
    isPaused = false;

    // Match configuration
    currentTime = 0;
    isStockMatch = true;
    isSuddenDeath = false;

    // Default settings
    settings.stockCount = DEFAULT_STOCKS;
    settings.timeLimit = 180; // 3 minutes
    settings.itemsEnabled = true;
    settings.itemFrequency = 0.5f;
    settings.stageHazards = true;
    settings.finalSmash = true;

    // UI selections
    titleOptionSelected = 0;
    for (int i = 0; i < 4; i++) {
        characterSelectIndex[i] = i % 2; // Default selection
    }
    stageSelectIndex = 0;

    // Debug mode
    debugMode = false;
    debugText = "";

    // Results
    winnerIndex = -1;
}

void GameState::initialize() {
    // Reset all players to their spawn points
    for (int i = 0; i < players.size() && i < spawnPoints.size(); i++) {
        players[i]->respawn(spawnPoints[i]);
    }

    // Clear items and particles
    items.clear();
    particles.clear();

    // Reset match timer
    currentTime = 0;
    stateTimer = 0;
    isSuddenDeath = false;

    // Reset results
    results.clear();
    winnerIndex = -1;
}

void GameState::update() {
    stateTimer++;

    if (currentState == GAME_PLAYING && !isPaused) {
        currentTime++;

        // Check for match time limit if in timed mode
        if (!isStockMatch && settings.timeLimit > 0) {
            if (isMatchTimeUp()) {
                if (getLeadingPlayer() != -1) {
                    endMatch();
                } else {
                    startSuddenDeath();
                }
            }
        }

        // Check for match end conditions (all but one player eliminated)
        if (isStockMatch && getRemainingPlayers() <= 1) {
            endMatch();
        }

        // Spawn items randomly
        if (settings.itemsEnabled && GetRandomValue(0, 100) < settings.itemFrequency * 100 && GetFrameTime() * GetRandomValue(0, 1000) < 1) {
            spawnRandomItem();
        }

        // Update items
        updateItems();
    }
}

void GameState::draw() {
    switch (currentState) {
        case TITLE_SCREEN:
            drawTitleScreen();
            break;

        case CHARACTER_SELECT:
            drawCharacterSelect();
            break;

        case STAGE_SELECT:
            drawStageSelect();
            break;

        case GAME_START:
        case GAME_PLAYING:
        case GAME_SUDDEN_DEATH:
        case GAME_OVER:
            drawGamePlaying();

            // Draw HUD on top
            drawHUD();
            break;

        case GAME_PAUSED:
            drawGamePlaying();
            drawGamePaused();
            break;

        case RESULTS_SCREEN:
            drawResultsScreen();
            break;
    }

    // Draw debug info if enabled
    if (debugMode) {
        drawDebugInfo();
    }
}

void GameState::changeState(State newState) {
    currentState = newState;
    stateTimer = 0;

    // State-specific initialization
    switch (newState) {
        case GAME_START:
            initialize();
            break;

        case GAME_PLAYING:
            isPaused = false;
            break;

        case GAME_PAUSED:
            isPaused = true;
            break;

        case GAME_SUDDEN_DEATH:
            startSuddenDeath();
            break;

        case GAME_OVER:
            processResults();
            break;

        default:
            break;
    }
}

void GameState::startMatch(const MatchSettings& matchSettings) {
    settings = matchSettings;
    isStockMatch = (settings.stockCount > 0);

    // Set stocks for all players
    for (auto& player : players) {
        player->stocks = settings.stockCount;
        player->damagePercent = 0;
    }

    changeState(GAME_START);
}

void GameState::endMatch() {
    changeState(GAME_OVER);
}

void GameState::pauseGame() {
    if (currentState == GAME_PLAYING) {
        changeState(GAME_PAUSED);
    }
}

void GameState::resumeGame() {
    if (currentState == GAME_PAUSED) {
        changeState(GAME_PLAYING);
    }
}

void GameState::resetMatch() {
    // Reset players
    for (auto& player : players) {
        player->stocks = settings.stockCount;
        player->damagePercent = 0;
        player->isDying = false;
    }

    // Clear items and effects
    items.clear();
    particles.clear();

    // Reset timers
    currentTime = 0;
    stateTimer = 0;

    // Return to title or start a new match
    changeState(TITLE_SCREEN);
}

void GameState::checkMatchEnd() {
    if (isStockMatch) {
        if (getRemainingPlayers() <= 1) {
            endMatch();
        }
    } else if (isMatchTimeUp()) {
        endMatch();
    }
}

void GameState::startSuddenDeath() {
    isSuddenDeath = true;

    // Reset all players to high damage
    for (auto& player : players) {
        if (player->stocks > 0) {
            player->damagePercent = 300.0f;
        }
    }

    changeState(GAME_SUDDEN_DEATH);
}

void GameState::processResults() {
    results.clear();

    // Gather player stats
    for (int i = 0; i < players.size(); i++) {
        PlayerResult result;
        result.name = (i == 0) ? "You" : "Enemy";
        result.stocksRemaining = players[i]->stocks;

        results.push_back(result);

        // Find winner
        if (players[i]->stocks > 0) {
            winnerIndex = i;
        }
    }
}

void GameState::respawnPlayer(int playerIndex) {
    if (playerIndex >= 0 && playerIndex < players.size() && players[playerIndex]->stocks > 0) {
        // Find an available spawn point
        int spawnIndex = playerIndex % spawnPoints.size();
        players[playerIndex]->respawn(spawnPoints[spawnIndex]);
    }
}

void GameState::spawnRandomItem() {
    // Not implemented yet
}

void GameState::updateItems() {
    // Item functionality removed - will be implemented in a future update
    items.clear(); // Just clear any existing items since we can't actually update them yet
}

void GameState::drawItems() {
    // Item drawing removed - will be implemented in a future update
}

void GameState::drawTitleScreen() {
    // Draw background
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});

    // Draw title
    DrawText("SUPER SMASH CLONE", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/4, 40, WHITE);

    // Draw menu options
    const char* options[] = {
        "START GAME",
        "OPTIONS",
        "CONTROLS",
        "EXIT"
    };

    for (int i = 0; i < 4; i++) {
        Color optionColor = (i == titleOptionSelected) ? RED : WHITE;
        DrawText(options[i], SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + i * 40, 24, optionColor);
    }

    // Draw controls info
    DrawText("Press ENTER to select", SCREEN_WIDTH/2 - 110, SCREEN_HEIGHT - 100, 20, WHITE);
    DrawText("Use UP/DOWN to navigate", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT - 70, 20, WHITE);
}

void GameState::drawCharacterSelect() {
    // Draw background
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {40, 40, 80, 255});

    // Draw title
    DrawText("CHARACTER SELECT", SCREEN_WIDTH/2 - 160, 50, 40, WHITE);

    // Draw character slots for 4 players
    for (int i = 0; i < 4; i++) {
        // Character box
        Rectangle charRect = {
            static_cast<float>(200 + (i % 2) * 450),
            static_cast<float>(150 + (i / 2) * 250),
            200,
            200
        };

        Color playerColor;
        switch (i) {
            case 0: playerColor = RED; break;
            case 1: playerColor = BLUE; break;
            case 2: playerColor = GREEN; break;
            case 3: playerColor = YELLOW; break;
            default: playerColor = WHITE;
        }

        DrawRectangleRec(charRect, playerColor);
        DrawRectangleLinesEx(charRect, 3, WHITE);

        // Player label
        DrawText(TextFormat("P%d", i+1), charRect.x + 85, charRect.y + 80, 30, WHITE);

        // Ready indicator for human players
        if (i < 2) { // First two are human players
            DrawText("READY!", charRect.x + 70, charRect.y + 140, 20, WHITE);
        }
    }

    // Draw navigation prompt
    DrawText("Press ENTER to continue", SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT - 80, 24, WHITE);
}

void GameState::drawStageSelect() {
    // Draw background
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {40, 80, 40, 255});

    // Draw title
    DrawText("STAGE SELECT", SCREEN_WIDTH/2 - 120, 50, 40, WHITE);

    // Draw stage options
    const char* stageNames[] = {
        "BATTLEFIELD",
        "FINAL DESTINATION",
        "DREAM LAND",
        "POKEMON STADIUM",
        "SMASHVILLE"
    };

    for (int i = 0; i < 5; i++) {
        Rectangle stageRect = {
            static_cast<float>(140 + (i % 3) * 350),
            static_cast<float>(150 + (i / 3) * 220),
            300,
            200
        };

        Color stageColor = (i == stageSelectIndex) ? GREEN : DARKGRAY;
        DrawRectangleRec(stageRect, stageColor);
        DrawRectangleLinesEx(stageRect, 3, WHITE);

        // Stage name
        DrawText(stageNames[i], stageRect.x + 150 - MeasureText(stageNames[i], 24)/2,
                stageRect.y + 85, 24, WHITE);
    }

    // Draw navigation prompt
    DrawText("Press ENTER to start match", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT - 80, 24, WHITE);
}

void GameState::drawHUD() {
    // Draw player stock icons and damage percentages
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

    // Draw match timer if time mode is enabled
    if (settings.timeLimit > 0) {
        int timeRemaining = settings.timeLimit - currentTime / 60;
        DrawText(
            TextFormat("%d:%02d", timeRemaining / 60, timeRemaining % 60),
            SCREEN_WIDTH / 2 - 40,
            HUD_MARGIN,
            40,
            WHITE
        );
    }

    // Draw sudden death indicator
    if (isSuddenDeath) {
        DrawText("SUDDEN DEATH!", SCREEN_WIDTH/2 - 120, 70, 30, RED);
    }
}

void GameState::drawGamePlaying() {
    // Game objects are drawn by the main drawing function

    // Draw additional game state info
    if (currentState == GAME_START) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 100});
        int countdown = (GAME_START_TIMER - stateTimer) / 60 + 1;
        DrawText(TextFormat("%d", countdown), SCREEN_WIDTH/2 - 20, SCREEN_HEIGHT/2 - 50, 100, WHITE);
    }

    if (currentState == GAME_OVER) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});

        // Find the winner
        int winnerId = winnerIndex;

        if (winnerId != -1) {
            DrawText(TextFormat("PLAYER %d WINS!", winnerId + 1), SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/3, 40, WHITE);
        } else {
            DrawText("DRAW!", SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/3, 40, WHITE);
        }
    }
}

void GameState::drawGamePaused() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});
    DrawText("PAUSED", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/3, 50, WHITE);
    DrawText("Press P to Resume", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2, 30, WHITE);
    DrawText("Press R to Restart", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 40, 30, WHITE);
}

void GameState::drawResultsScreen() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});
    DrawText("RESULTS", SCREEN_WIDTH/2 - 80, 100, 40, WHITE);

    for (int i = 0; i < results.size(); i++) {
        Color playerColor = players[i]->color;
        
        // Change display names based on PvE
        std::string displayName = (i == 0) ? "You" : "Enemy";
        
        DrawText(displayName.c_str(), 200, 200 + i*80, 30, playerColor);
        DrawText(TextFormat("Stocks: %d", results[i].stocksRemaining), 400, 200 + i*80, 30, WHITE);

        if (winnerIndex == i) {
            if (i == 0) {
                DrawText("VICTORY!", 600, 200 + i*80, 30, GREEN);
            } else {
                DrawText("DEFEATED YOU!", 600, 200 + i*80, 30, RED);
            }
        }
    }

    DrawText("Press ENTER to return to title screen", SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT - 100, 24, WHITE);
}

void GameState::drawDebugInfo() {
    DrawRectangle(10, 10, 300, 100, {0, 0, 0, 128});
    DrawText(TextFormat("State: %d | Timer: %d", currentState, stateTimer), 20, 20, 16, WHITE);
    DrawText(TextFormat("FPS: %d | Players: %d", GetFPS(), (int)players.size()), 20, 40, 16, WHITE);
    DrawText(TextFormat("Items: %d | Particles: %d", (int)items.size(), (int)particles.size()), 20, 60, 16, WHITE);
    DrawText(debugText.c_str(), 20, 80, 16, YELLOW);
}

void GameState::toggleDebugMode() {
    debugMode = !debugMode;
}

bool GameState::isMatchTimeUp() {
    return settings.timeLimit > 0 && (currentTime / 60 >= settings.timeLimit);
}

bool GameState::checkAllPlayersDead() {
    for (auto& player : players) {
        if (player->stocks > 0) {
            return false;
        }
    }
    return true;
}

int GameState::getLeadingPlayer() {
    if (isStockMatch) {
        // In stock match, the player with most stocks is leading
        int maxStocks = -1;
        int leadingPlayer = -1;
        bool tie = false;

        for (int i = 0; i < players.size(); i++) {
            if (players[i]->stocks > maxStocks) {
                maxStocks = players[i]->stocks;
                leadingPlayer = i;
                tie = false;
            } else if (players[i]->stocks == maxStocks) {
                tie = true;
            }
        }

        return tie ? -1 : leadingPlayer;
    } else {
        // For time match, we would track KOs, but not implemented yet
        return -1;
    }
}

int GameState::getRemainingPlayers() {
    int count = 0;
    for (auto& player : players) {
        if (player->stocks > 0) {
            count++;
        }
    }
    return count;
}