#include "Particle.h"

Particle::Particle(Vector2 pos, Vector2 vel, float s, int life, Color col) {
    position = pos;
    velocity = vel;
    size = s;
    lifespan = life;
    currentLife = 0;
    color = col;
}

bool Particle::update() {
    // Update position based on velocity
    position.x += velocity.x;
    position.y += velocity.y;

    // Apply gravity or drag as needed
    velocity.y += 0.1f;  // Light gravity
    velocity.x *= 0.98f; // Air resistance

    // Update lifespan
    currentLife++;

    // Shrink particle slightly over time
    size = size * 0.98f;

    // Return true if particle is still alive
    return currentLife < lifespan;
}

void Particle::draw() {
    // Calculate alpha based on remaining life
    unsigned char alpha = (unsigned char)(255 * (1.0f - (float)currentLife / lifespan));
    Color drawColor = {color.r, color.g, color.b, alpha};

    // Draw particle as circle
    DrawCircleV(position, size, drawColor);
}