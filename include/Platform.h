#ifndef PLATFORM_H
#define PLATFORM_H

#include "raylib.h"
#include <algorithm> // Add this for std::min

// Platform types to support different platform behaviors
enum PlatformType {
    SOLID,      // Collide from all sides (walls, ground)
    PASSTHROUGH // Standard platform - only collide from above
};

// Platform class with enhanced behavior options
class Platform {
public:
    Rectangle rect;
    Color color;
    PlatformType type;

    // Constructor with platform type parameter (defaults to PASSTHROUGH)
    Platform(float x, float y, float width, float height, Color col, PlatformType platformType = PASSTHROUGH);

    // Draw method declaration
    void draw();
};

#endif // PLATFORM_H