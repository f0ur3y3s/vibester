#include "raylib.h"
#include "character/Character.h"
#include "Platform.h"
#include "Particle.h"
#include "GameConfig.h"
#include "NetworkedGameState.h" // Use NetworkedGameState for both networked and local play
#include "NetworkUI.h"          // Network UI components
#include "EnhancedAIController.h" // Enhanced AI controller for PvE
#include "character/CharacterVisuals.h"
#include "StateManager.h"
#include <vector>
#include <string>
#include <iostream>
#include <ctime>
#include <random>

// Background class definition - add this to your existing declarations
class Background
{
private:
    // Background layers
    struct CloudLayer
    {
        std::vector<Rectangle> clouds;
        Color color;
        float speed;
        float y;
    };

    std::vector<CloudLayer> cloudLayers;
    Color skyTopColor;
    Color skyBottomColor;

    // Distant elements (mountains, hills)
    struct DistantElement
    {
        std::vector<Vector2> points;
        Color color;
    };

    std::vector<DistantElement> distantElements;

    // Background theme type
    enum BackgroundTheme
    {
        THEME_SKY,
        THEME_SUNSET,
        THEME_NIGHT,
        THEME_BATTLEFIELD
    };

    BackgroundTheme currentTheme;
    float time; // For animations

    // Random generator
    std::mt19937 rng;

public:
    Background() : time(0)
    {
        // Seed random generator
        rng.seed(static_cast<unsigned int>(std::time(nullptr)));

        // Initialize with default theme
        setRandomTheme();
    }

    void setRandomTheme()
    {
        // Pick a random theme
        std::uniform_int_distribution<int> themeDist(0, 3);
        currentTheme = static_cast<BackgroundTheme>(themeDist(rng));

        // Clear previous elements
        cloudLayers.clear();
        distantElements.clear();

        // Set colors based on theme
        switch (currentTheme)
        {
        case THEME_SKY:
            skyTopColor = {100, 181, 246, 255}; // Light blue
            skyBottomColor = {179, 229, 252, 255}; // Very light blue
            generateSkyTheme();
            break;

        case THEME_SUNSET:
            skyTopColor = {33, 150, 243, 255}; // Deep blue
            skyBottomColor = {255, 152, 0, 255}; // Orange
            generateSunsetTheme();
            break;

        case THEME_NIGHT:
            skyTopColor = {25, 25, 112, 255}; // Midnight blue
            skyBottomColor = {48, 63, 159, 255}; // Indigo
            generateNightTheme();
            break;

        case THEME_BATTLEFIELD:
            skyTopColor = {33, 33, 33, 255}; // Dark gray
            skyBottomColor = {97, 97, 97, 255}; // Medium gray
            generateBattlefieldTheme();
            break;
        }
    }

private:
    void generateSkyTheme()
    {
        // Generate 3 cloud layers
        std::uniform_real_distribution<float> speedDist(0.2f, 1.0f);

        // White fluffy clouds
        CloudLayer layer1;
        layer1.color = {255, 255, 255, 180};
        layer1.speed = speedDist(rng);
        layer1.y = SCREEN_HEIGHT * 0.25f;
        generateCloudsForLayer(layer1, 5, 8);
        cloudLayers.push_back(layer1);

        // Light gray distant clouds
        CloudLayer layer2;
        layer2.color = {240, 240, 240, 150};
        layer2.speed = speedDist(rng) * 0.6f;
        layer2.y = SCREEN_HEIGHT * 0.35f;
        generateCloudsForLayer(layer2, 3, 6);
        cloudLayers.push_back(layer2);

        // Generate distant hills
        DistantElement hills;
        hills.color = {46, 125, 50, 200}; // Green
        generateHills(hills, SCREEN_HEIGHT * 0.65f, SCREEN_HEIGHT * 0.2f, 3);
        distantElements.push_back(hills);
    }

    void generateSunsetTheme()
    {
        // Generate orange-tinted clouds
        std::uniform_real_distribution<float> speedDist(0.1f, 0.8f);

        // Orange-tinted clouds
        CloudLayer layer1;
        layer1.color = {255, 183, 77, 180};
        layer1.speed = speedDist(rng);
        layer1.y = SCREEN_HEIGHT * 0.2f;
        generateCloudsForLayer(layer1, 6, 10);
        cloudLayers.push_back(layer1);

        // Darker orange distant clouds
        CloudLayer layer2;
        layer2.color = {251, 140, 0, 150};
        layer2.speed = speedDist(rng) * 0.5f;
        layer2.y = SCREEN_HEIGHT * 0.3f;
        generateCloudsForLayer(layer2, 4, 7);
        cloudLayers.push_back(layer2);

        // Generate distant mountains
        DistantElement mountains;
        mountains.color = {69, 39, 160, 230}; // Deep purple
        generateMountains(mountains, SCREEN_HEIGHT * 0.7f, SCREEN_HEIGHT * 0.25f, 5);
        distantElements.push_back(mountains);
    }

    void generateNightTheme()
    {
        // Generate star field
        DistantElement stars;
        stars.color = {255, 255, 255, 255};
        generateStars(stars, 100);
        distantElements.push_back(stars);

        // Generate thin clouds
        std::uniform_real_distribution<float> speedDist(0.05f, 0.4f);

        // Dark blue clouds
        CloudLayer layer1;
        layer1.color = {40, 53, 147, 100};
        layer1.speed = speedDist(rng);
        layer1.y = SCREEN_HEIGHT * 0.2f;
        generateCloudsForLayer(layer1, 4, 6);
        cloudLayers.push_back(layer1);

        // Generate distant mountains
        DistantElement mountains;
        mountains.color = {26, 35, 126, 255}; // Very dark blue
        generateMountains(mountains, SCREEN_HEIGHT * 0.75f, SCREEN_HEIGHT * 0.3f, 4);
        distantElements.push_back(mountains);
    }

    void generateBattlefieldTheme()
    {
        // Generate tech-looking elements
        std::uniform_real_distribution<float> speedDist(0.1f, 0.3f);

        // Generate mesh/grid lines
        DistantElement gridlines;
        gridlines.color = {0, 229, 255, 100}; // Cyan
        generateGridlines(gridlines, 20);
        distantElements.push_back(gridlines);

        // Light smoke/clouds
        CloudLayer layer1;
        layer1.color = {200, 200, 200, 80};
        layer1.speed = speedDist(rng);
        layer1.y = SCREEN_HEIGHT * 0.3f;
        generateCloudsForLayer(layer1, 5, 7);
        cloudLayers.push_back(layer1);

        // Generate tech structures
        DistantElement structures;
        structures.color = {66, 66, 66, 230}; // Dark gray
        generateTechStructures(structures);
        distantElements.push_back(structures);
    }

    void generateCloudsForLayer(CloudLayer& layer, int minClouds, int maxClouds)
    {
        std::uniform_int_distribution<int> cloudCountDist(minClouds, maxClouds);
        std::uniform_real_distribution<float> widthDist(80.0f, 300.0f);
        std::uniform_real_distribution<float> heightDist(30.0f, 80.0f);
        std::uniform_real_distribution<float> xPosDist(0.0f, SCREEN_WIDTH * 2);

        int cloudCount = cloudCountDist(rng);

        for (int i = 0; i < cloudCount; i++)
        {
            Rectangle cloud;
            cloud.width = widthDist(rng);
            cloud.height = heightDist(rng);
            cloud.x = xPosDist(rng) - SCREEN_WIDTH * 0.5f;
            cloud.y = layer.y + heightDist(rng) * 0.3f;

            layer.clouds.push_back(cloud);
        }
    }

    void generateHills(DistantElement& hills, float baseY, float height, int segments)
    {
        std::uniform_real_distribution<float> heightDist(0.5f, 1.0f);

        float segmentWidth = SCREEN_WIDTH / static_cast<float>(segments);

        // Start point
        hills.points.push_back({0, baseY});

        // Generate hill points
        for (int i = 0; i <= segments; i++)
        {
            float x = i * segmentWidth;
            float y = baseY - height * heightDist(rng);
            hills.points.push_back({x, y});
        }

        // End point
        hills.points.push_back({SCREEN_WIDTH, baseY});
    }

    void generateMountains(DistantElement& mountains, float baseY, float height, int peaks)
    {
        std::uniform_real_distribution<float> heightDist(0.6f, 1.0f);
        std::uniform_real_distribution<float> widthDist(0.8f, 1.2f);

        float segmentWidth = SCREEN_WIDTH / static_cast<float>(peaks);

        // Start point
        mountains.points.push_back({0, baseY});

        // Generate mountain peaks
        for (int i = 0; i < peaks; i++)
        {
            float centerX = (i + 0.5f) * segmentWidth;
            float peakHeight = height * heightDist(rng);
            float leftX = centerX - segmentWidth * 0.25f * widthDist(rng);
            float rightX = centerX + segmentWidth * 0.25f * widthDist(rng);

            // Left slope
            mountains.points.push_back({leftX, baseY - peakHeight * 0.3f});

            // Peak
            mountains.points.push_back({centerX, baseY - peakHeight});

            // Right slope
            mountains.points.push_back({rightX, baseY - peakHeight * 0.3f});
        }

        // End point
        mountains.points.push_back({SCREEN_WIDTH, baseY});
    }

    void generateStars(DistantElement& stars, int count)
    {
        std::uniform_real_distribution<float> xDist(0.0f, SCREEN_WIDTH);
        std::uniform_real_distribution<float> yDist(0.0f, SCREEN_HEIGHT * 0.6f);

        for (int i = 0; i < count; i++)
        {
            stars.points.push_back({xDist(rng), yDist(rng)});
        }
    }

    void generateGridlines(DistantElement& gridlines, int count)
    {
        std::uniform_real_distribution<float> yDist(0.0f, SCREEN_HEIGHT * 0.8f);
        std::uniform_real_distribution<float> lengthDist(100.0f, SCREEN_WIDTH * 0.8f);
        std::uniform_real_distribution<float> xDist(0.0f, SCREEN_WIDTH);

        for (int i = 0; i < count; i++)
        {
            float x = xDist(rng);
            float y = yDist(rng);
            float length = lengthDist(rng);

            // Horizontal or vertical line
            if (i % 2 == 0)
            {
                // Horizontal
                gridlines.points.push_back({x, y});
                gridlines.points.push_back({x + length, y});
            }
            else
            {
                // Vertical
                gridlines.points.push_back({x, y});
                gridlines.points.push_back({x, y + length});
            }
        }
    }

    void generateTechStructures(DistantElement& structures)
    {
        std::uniform_real_distribution<float> widthDist(40.0f, 120.0f);
        std::uniform_real_distribution<float> heightDist(50.0f, 200.0f);
        std::uniform_int_distribution<int> countDist(5, 15);
        std::uniform_real_distribution<float> xDist(0.0f, SCREEN_WIDTH);

        int count = countDist(rng);
        float baseY = SCREEN_HEIGHT * 0.75f;

        for (int i = 0; i < count; i++)
        {
            float x = xDist(rng);
            float width = widthDist(rng);
            float height = heightDist(rng);

            // Building structure
            structures.points.push_back({x, baseY});
            structures.points.push_back({x, baseY - height});
            structures.points.push_back({x + width, baseY - height});
            structures.points.push_back({x + width, baseY});

            // Extra point as separator between buildings
            structures.points.push_back({-1, -1});
        }
    }

public:
    void update(float deltaTime)
    {
        time += deltaTime;

        // Update cloud positions
        for (auto& layer : cloudLayers)
        {
            for (auto& cloud : layer.clouds)
            {
                cloud.x += layer.speed;

                // Wrap around when outside screen
                if (cloud.x > SCREEN_WIDTH + cloud.width)
                {
                    cloud.x = -cloud.width;
                }
            }
        }
    }

    void draw()
    {
        // Draw sky gradient
        DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, skyTopColor, skyBottomColor);

        // Draw distant elements
        for (const auto& element : distantElements)
        {
            // Stars
            if (currentTheme == THEME_NIGHT && element.color.r == 255 && element.color.g == 255 && element.color.b ==
                255)
            {
                for (const auto& point : element.points)
                {
                    // Twinkle effect
                    float brightness = 0.7f + 0.3f * sin(time * 2.0f + point.x * 0.1f + point.y * 0.1f);
                    Color starColor = {
                        element.color.r,
                        element.color.g,
                        element.color.b,
                        static_cast<unsigned char>(255 * brightness)
                    };
                    DrawCircle(point.x, point.y, 1.0f, starColor);
                }
            }
            // Grid lines
            else if (currentTheme == THEME_BATTLEFIELD && element.color.r == 0 && element.color.g == 229)
            {
                for (size_t i = 0; i < element.points.size(); i += 2)
                {
                    if (i + 1 < element.points.size())
                    {
                        DrawLineEx(
                            element.points[i],
                            element.points[i + 1],
                            1.0f,
                            element.color
                        );
                    }
                }
            }
            // Tech structures
            else if (currentTheme == THEME_BATTLEFIELD && element.color.r == 66)
            {
                Vector2 buildingPoints[4];
                int pointIndex = 0;

                for (size_t i = 0; i < element.points.size(); i++)
                {
                    if (element.points[i].x < 0)
                    {
                        // Separator, draw previous building
                        if (pointIndex == 4)
                        {
                            DrawTriangle(
                                buildingPoints[0],
                                buildingPoints[1],
                                buildingPoints[2],
                                element.color
                            );
                            DrawTriangle(
                                buildingPoints[0],
                                buildingPoints[2],
                                buildingPoints[3],
                                element.color
                            );
                        }
                        pointIndex = 0;
                    }
                    else
                    {
                        if (pointIndex < 4)
                        {
                            buildingPoints[pointIndex++] = element.points[i];
                        }
                    }
                }
            }
            // Hills and mountains
            else
            {
                Vector2* points = new Vector2[element.points.size() + 2];
                int pointCount = 0;

                for (const auto& point : element.points)
                {
                    points[pointCount++] = point;
                }

                // Add bottom corners to close the shape
                points[pointCount++] = {SCREEN_WIDTH, SCREEN_HEIGHT};
                points[pointCount++] = {0, SCREEN_HEIGHT};

                DrawTriangleFan(points, pointCount, element.color);

                delete[] points;
            }
        }

        // Draw clouds
        for (const auto& layer : cloudLayers)
        {
            for (const auto& cloud : layer.clouds)
            {
                // Draw multiple circles for fluffy cloud effect
                float baseRadius = cloud.height * 0.5f;

                // Center cloud
                DrawCircle(
                    cloud.x + cloud.width * 0.5f,
                    cloud.y + cloud.height * 0.5f,
                    baseRadius,
                    layer.color
                );

                // Left puff
                DrawCircle(
                    cloud.x + cloud.width * 0.25f,
                    cloud.y + cloud.height * 0.6f,
                    baseRadius * 0.8f,
                    layer.color
                );

                // Right puff
                DrawCircle(
                    cloud.x + cloud.width * 0.75f,
                    cloud.y + cloud.height * 0.6f,
                    baseRadius * 0.8f,
                    layer.color
                );

                // Top puff
                DrawCircle(
                    cloud.x + cloud.width * 0.5f,
                    cloud.y + cloud.height * 0.3f,
                    baseRadius * 0.7f,
                    layer.color
                );
            }
        }
    }
};

// Add this to your global game variables
Background background;

// Add methods to change the theme
void CycleBackgroundTheme()
{
    background.setRandomTheme();
}

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
NetworkedGameState gameState; // Use NetworkedGameState for both modes
NetworkUI* networkUI = nullptr; // Network UI
bool showNetworkMenu = false; // Flag to show/hide network menu
std::vector<Character*> players;
std::vector<Platform> platforms;
std::vector<Vector2> spawnPoints;
std::vector<Particle> particles;
Font gameFont;
bool debugMode = false;

// Create an instance of the enhanced AI controller for PvE
std::unique_ptr<EnhancedAIController> enhancedAI;
float difficultyLevel = 0.8f; // Default to challenging (0.0 to 1.0)

// Main entry point
int main()
{
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Super Smash Clone - Ultimate Edition");
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
    background = Background();
    CharacterVisuals::InitShaders();

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

    // Create player and enemy/opponent
    Character* player1 = new Character(
        spawnPoints[0].x, spawnPoints[0].y,
        50, 80,
        5.0f,
        RED,
        "Player 1",
        STYLE_BRAWLER
    );

    // Create enemy/opponent with different style
    Character* player2 = new Character(
        spawnPoints[1].x, spawnPoints[1].y,
        50, 80,
        5.0f,
        BLUE,
        "Player 2",
        STYLE_SPEEDY // Fox-like character
    );

    players.push_back(player1);
    players.push_back(player2);

    // Initialize the AI controller for PvE
    enhancedAI = std::make_unique<EnhancedAIController>();
    enhancedAI->SetDifficulty(difficultyLevel);

    // Initialize networked game state (works for both modes)
    gameState = NetworkedGameState();
    gameState.players = players;
    gameState.platforms = platforms;
    gameState.spawnPoints = spawnPoints;

    // Create the network UI
    networkUI = new NetworkUI(&gameState);

    // Start in title screen
    gameState.currentState = GameState::TITLE_SCREEN;

    // Setup default match settings
    gameState.settings.stockCount = DEFAULT_STOCKS;
    gameState.settings.timeLimit = 180; // 3 minutes
    gameState.settings.itemsEnabled = true;
    gameState.settings.itemFrequency = 0.5f;
    gameState.settings.stageHazards = true;
    gameState.settings.finalSmash = true;

    // Initialize NetworkManager
    NetworkManager::getInstance().initialize();
}

void UpdateGame()
{
    // Check for network menu toggle
    if (IsKeyPressed(KEY_N))
    {
        // Only toggle if not in active gameplay
        if (gameState.currentState != GameState::GAME_PLAYING &&
            gameState.currentState != GameState::GAME_START)
        {
            showNetworkMenu = !showNetworkMenu;
        }
    }

    // Always update networked game state first to check for messages,
    // even if we're in network UI mode
    if (gameState.isNetworked())
    {
        // Process network messages
        NetworkManager& netManager = NetworkManager::getInstance();
        netManager.update();

        // Check explicitly for game start message when in client mode
        if (gameState.getNetworkMode() == NetworkedGameState::CLIENT)
        {
            bool gameStarted = netManager.hasGameStartMessage();
            if (gameStarted &&
                gameState.currentState != GameState::GAME_PLAYING &&
                gameState.currentState != GameState::GAME_START)
            {
                std::cout << "Game.cpp: Client detected game start message!" << std::endl;
                showNetworkMenu = false;
                gameState.changeState(GameState::GAME_START);
            }
        }
    }

    // Update network UI if visible
    if (showNetworkMenu)
    {
        networkUI->update();

        // IMPORTANT: If the game state is now GAME_START or GAME_PLAYING, we need to hide the menu
        if (gameState.currentState == GameState::GAME_PLAYING ||
            gameState.currentState == GameState::GAME_START)
        {
            showNetworkMenu = false;
        }
        else
        {
            return; // Skip normal game update when in network menu
        }
    }

    // Check if we're in networked mode
    bool isNetworked = gameState.isNetworked();

    // Process game state
    switch (gameState.currentState)
    {
    case GameState::TITLE_SCREEN:
        // Character style selection
        if (IsKeyPressed(KEY_ONE) && players.size() >= 2)
        {
            // Cycle player 1 style
            int currentStyle = static_cast<int>(players[0]->characterStyle);
            currentStyle = (currentStyle + 1) % 5; // 5 styles total

            players[0]->characterStyle = static_cast<CharacterStyle>(currentStyle);

            // Recreate visuals with new style
            delete players[0]->visuals;
            players[0]->visuals = new
                CharacterVisuals(players[0], players[0]->characterStyle, players[0]->color, WHITE);
        }

        if (IsKeyPressed(KEY_TWO) && players.size() >= 2)
        {
            // Cycle player 2 style
            int currentStyle = static_cast<int>(players[1]->characterStyle);
            currentStyle = (currentStyle + 1) % 5; // 5 styles total

            players[1]->characterStyle = static_cast<CharacterStyle>(currentStyle);

            // Recreate visuals with new style
            delete players[1]->visuals;
            players[1]->visuals = new
                CharacterVisuals(players[1], players[1]->characterStyle, players[1]->color, WHITE);
        }

    // Difficulty selection (keep your existing code)
        if (IsKeyPressed(KEY_ONE)) difficultyLevel = 0.2f;
        if (IsKeyPressed(KEY_TWO)) difficultyLevel = 0.5f;
        if (IsKeyPressed(KEY_THREE)) difficultyLevel = 0.8f;
        if (IsKeyPressed(KEY_FOUR)) difficultyLevel = 1.0f;

    // Background cycle
        if (IsKeyPressed(KEY_B))
        {
            CycleBackgroundTheme();
        }

    // Game start
        if (IsKeyPressed(KEY_ENTER))
        {
            gameState.changeState(GameState::GAME_START);
        }
        break;

    case GameState::GAME_START:
        // Hide network menu if it's open when the game is starting
        if (showNetworkMenu)
        {
            std::cout << "Game.cpp: Hiding network menu for game start" << std::endl;
            showNetworkMenu = false;
        }

    // Game start countdown
        gameState.stateTimer++;
        std::cout << "Game.cpp: Game start countdown: " << gameState.stateTimer << "/" << GAME_START_TIMER << std::endl;
        if (gameState.stateTimer >= GAME_START_TIMER)
        {
            std::cout << "Game.cpp: Countdown finished, changing to GAME_PLAYING" << std::endl;
            gameState.changeState(GameState::GAME_PLAYING);
        }

    // Update player positions during countdown
        if (isNetworked)
        {
            gameState.update();
        }
        break;

    case GameState::GAME_PLAYING:
        // Check for pause
        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
        {
            gameState.pauseGame();
            break;
        }

        background.update(GetFrameTime());

    // Update debug mode
        if (IsKeyPressed(KEY_F1))
        {
            debugMode = !debugMode;
        }

    // For networked mode, use the NetworkedGameState update which handles sync
        if (isNetworked)
        {
            gameState.update();
        }
        else
        {
            // Normal update for local play

            // Update players
            for (auto& player : players)
            {
                player->update(platforms);
            }

            // Check for character collisions for attacks
            for (auto& attacker : players)
            {
                // Skip players who are dying or exploding
                if (attacker->stateManager.isDying || attacker->stateManager.isExploding)
                {
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

            // Player 1 controls (human)
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

            // Only run AI in local PvE mode (not in networked mode)
            if (players[1]->stocks > 0 && !players[1]->stateManager.isDying)
            {
                // Run the enhanced AI for the enemy
                enhancedAI->Update(players, platforms);
            }

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

        if (IsKeyPressed(KEY_B))
        {
            CycleBackgroundTheme();
        }

    // Network disconnect option in pause menu
        if (IsKeyPressed(KEY_N) && isNetworked)
        {
            gameState.disconnectFromGame();
            gameState.resumeGame();
            gameState.changeState(GameState::TITLE_SCREEN);
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
            // If in network mode, return to lobby
            if (isNetworked)
            {
                showNetworkMenu = true;
            }
            else
            {
                gameState.resetMatch();
                gameState.changeState(GameState::TITLE_SCREEN);
            }
        }
        break;

    default:
        break;
    }
}

void DrawGame()
{
    // Draw background
    background.draw();

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

    // Network status indicator
    if (gameState.isNetworked())
    {
        // Special case: Force client into game mode if host is in game mode
        // This is a fallback in case the MSG_GAME_START message was lost
        if (gameState.isNetworked() &&
            !gameState.isNetworkHost() &&
            gameState.currentState != GameState::GAME_START &&
            gameState.currentState != GameState::GAME_PLAYING &&
            showNetworkMenu)
        {
            // Check if the host is in game state by looking at the game state packet
            for (const auto& peer : NetworkManager::getInstance().peers)
            {
                if (peer.playerID == 0)
                {
                    // Host is always ID 0
                    // If host is in game mode, the ping will be active
                    bool hostInGameState = (peer.lastPingTime > 0);
                    if (hostInGameState)
                    {
                        std::cout << "EMERGENCY OVERRIDE: Detected host in game state, forcing client to start game" <<
                            std::endl;
                        showNetworkMenu = false;
                        gameState.changeState(GameState::GAME_START);
                    }
                    break;
                }
            }
        }

        Color statusColor = gameState.isNetworkHost() ? GREEN : BLUE;
        const char* statusText = gameState.isNetworkHost() ? "HOST" : "CLIENT";
        DrawText(statusText, SCREEN_WIDTH - 80, 10, 20, statusColor);

        // Display ping
        int ping = gameState.getAveragePing();
        Color pingColor = (ping < 50) ? GREEN : (ping < 100) ? YELLOW : RED;
        DrawText(TextFormat("Ping: %d ms", ping), SCREEN_WIDTH - 150, 35, 16, pingColor);

        // Show "Press N for Network Menu" text
        DrawText("Press N for Network Menu", SCREEN_WIDTH - 200, SCREEN_HEIGHT - 30, 16, WHITE);
    }

    // Draw state-specific screens
    switch (gameState.currentState)
    {
    case GameState::TITLE_SCREEN:
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 150});
            DrawText("SUPER SMASH CLONE - ULTIMATE EDITION", SCREEN_WIDTH / 2 - 320, SCREEN_HEIGHT / 4, 40, WHITE);
            DrawText("Press ENTER to Start Local Game", SCREEN_WIDTH / 2 - 220, SCREEN_HEIGHT / 2, 30, WHITE);
            DrawText("Press N for Network Play", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2 + 50, 30, WHITE);
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

            DrawText("Can you defeat the AI or challenge your friends online?",
                     SCREEN_WIDTH / 2 - 270, SCREEN_HEIGHT - 80, 20, WHITE);

            // Add the background theme info
            DrawText("Press B to change background theme", SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 80, 20, WHITE);
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

            // Show network disconnect option if in network mode
            if (gameState.isNetworked())
            {
                DrawText("Press N to Disconnect from Network", SCREEN_WIDTH / 2 - 220, SCREEN_HEIGHT / 2 + 80, 30, RED);
            }
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
                // Different message based on mode (AI or human opponent)
                if (gameState.isNetworked())
                {
                    DrawText("OPPONENT WINS!", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 3, 50, RED);
                }
                else
                {
                    DrawText("AI WINS!", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 3, 50, RED);
                }
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
                std::string displayName;

                if (gameState.isNetworked())
                {
                    displayName = (i == 0) ? "You" : "Opponent";
                }
                else
                {
                    displayName = (i == 0) ? "You" : "AI";
                }

                DrawText(displayName.c_str(), 200, 200 + i * 80, 30, playerColor);
                DrawText(TextFormat("Stocks: %d", players[i]->stocks), 400, 200 + i * 80, 30, WHITE);
                DrawText(TextFormat("Damage: %.0f%%", players[i]->damagePercent), 600, 200 + i * 80, 30, WHITE);
            }

            if (gameState.isNetworked())
            {
                DrawText("Press ENTER to return to lobby", SCREEN_WIDTH / 2 - 220, SCREEN_HEIGHT - 100, 24, WHITE);
            }
            else
            {
                DrawText("Press ENTER to return to title screen", SCREEN_WIDTH / 2 - 220, SCREEN_HEIGHT - 100, 24,
                         WHITE);
            }
        }
        break;
    }

    // Draw network UI if visible AND we're not in game start/play state
    if (showNetworkMenu && networkUI &&
        (gameState.currentState != GameState::GAME_START &&
            gameState.currentState != GameState::GAME_PLAYING))
    {
        networkUI->draw();
    }
    else if (gameState.currentState == GameState::GAME_START ||
        gameState.currentState == GameState::GAME_PLAYING)
    {
        // Force hide UI during game states
        showNetworkMenu = false;
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

            // AI state if applicable and not in network mode
            if (i == 1 && !gameState.isNetworked())
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
            TextFormat("FPS: %d | Particles: %d | Difficulty: %.1f | Network: %s",
                       GetFPS(), (int)particles.size(), difficultyLevel,
                       gameState.isNetworked() ? (gameState.isNetworkHost() ? "HOST" : "CLIENT") : "OFF"),
            10, SCREEN_HEIGHT - 40,
            16,
            WHITE
        );

        // If networked, show additional debug info
        if (gameState.isNetworked())
        {
            DrawText(
                TextFormat("Ping: %d ms | Sync: %.1f%% | Frame Adv: %d",
                           gameState.getAveragePing(),
                           gameState.getSyncPercentage(),
                           gameState.getFrameAdvantage()),
                10, SCREEN_HEIGHT - 20,
                16,
                WHITE
            );
        }
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

    // Shut down networking
    NetworkManager::getInstance().shutdown();

    // Delete network UI
    if (networkUI)
    {
        delete networkUI;
        networkUI = nullptr;
    }
}
