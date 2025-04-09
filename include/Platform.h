#ifndef PLATFORM_H
#define PLATFORM_H

#include "raylib.h"

// Platform class - now as a class rather than a struct for consistency
class Platform {
public:
    Rectangle rect;
    Color color;
    
    Platform(float x, float y, float width, float height, Color col);
    
    void draw();
};

#endif // PLATFORM_H