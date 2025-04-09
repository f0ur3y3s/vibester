#ifndef PARTICLE_H
#define PARTICLE_H

#include "raylib.h"

// Particle system
struct Particle {
    Vector2 position;
    Vector2 velocity;
    float size;
    int lifespan;
    int currentLife;
    Color color;
    
    Particle(Vector2 pos, Vector2 vel, float s, int life, Color col);
    
    bool update();
    void draw();
};

#endif // PARTICLE_H