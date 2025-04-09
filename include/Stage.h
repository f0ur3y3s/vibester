#ifndef STAGE_H
#define STAGE_H

#include "raylib.h"
#include "Platform.h"
#include "Character.h"
#include <string>
#include <vector>

// Stage hazard base class
class StageHazard {
public:
    enum HazardType {
        MOVING_PLATFORM,
        DAMAGING_AREA,
        WIND_AREA,
        WATER_AREA,
        CONVEYOR,
        LAUNCHER,
        FALLING_OBJECT,
        TRANSFORMATION
    };

    HazardType type;
    bool isActive;
    int activationTimer;
    int duration;
    Rectangle area;

    StageHazard(HazardType t, Rectangle r, int timer, int dur);

    virtual void update(std::vector<Character*>& characters);
    virtual void draw();
    virtual void activate();
    virtual void deactivate();
};

// Specific hazard types
class MovingPlatform : public StageHazard {
public:
    Platform platform;
    Vector2 startPos;
    Vector2 endPos;
    float speed;
    bool isLooping;
    bool isReversing;

    MovingPlatform(Platform p, Vector2 start, Vector2 end, float spd, int timer);

    void update(std::vector<Character*>& characters) override;
    void draw() override;
};

class DamagingArea : public StageHazard {
public:
    float damage;
    float knockback;
    int damageInterval;
    int currentInterval;
    Color effectColor;

    DamagingArea(Rectangle area, float dmg, float kb, int interval, int timer);

    void update(std::vector<Character*>& characters) override;
    void draw() override;
};

// Stage class
class Stage {
public:
    enum StageType {
        BATTLEFIELD,
        FINAL_DESTINATION,
        DREAM_LAND,
        POKEMON_STADIUM,
        SMASHVILLE,
        CUSTOM
    };

    StageType type;
    std::string name;
    Rectangle bounds;              // Main stage area
    Rectangle blastZones;          // Blast zones (death boundaries)
    std::vector<Platform> platforms;
    std::vector<StageHazard*> hazards;
    std::vector<Vector2> spawnPoints;

    // Visual properties
    Texture2D background;
    Texture2D foreground;
    Color bgColor;
    Color fgColor;
    bool hasParallaxBg;
    float parallaxFactor;

    // Dynamic stage properties
    bool canTransform;             // For stages like Pokémon Stadium
    int transformTimer;
    int transformDuration;
    int currentTransformation;
    bool hazardsEnabled;

    // Stage music
    Sound music;
    float musicVolume;
    bool musicPlaying;

    // Constructor
    Stage(StageType type, std::string name);

    // Core methods
    void initialize();
    void update(std::vector<Character*>& characters);
    void draw();
    void drawBackground();
    void drawForeground();

    // Hazard methods
    void toggleHazards(bool enabled);
    void activateRandomHazard();
    void updateHazards(std::vector<Character*>& characters);

    // Transformation methods
    void transform(int newTransform);
    void resetTransform();

    // Platform methods
    void addPlatform(float x, float y, float width, float height, Color color);
    void addMovingPlatform(float x, float y, float width, float height,
                          Vector2 endPos, float speed, Color color);

    // Hazard methods
    void addHazard(StageHazard* hazard);

    // Stage factory method
    static Stage* createStage(StageType type);
};

#endif // STAGE_H