#include "Particle.h"

Particle::Particle(Vector2 pos, Vector2 vel, float s, int life, Color col) : 
    position(pos), velocity(vel), size(s), lifespan(life), currentLife(life), color(col) {}

bool Particle::update() {
    position.x += velocity.x;
    position.y += velocity.y;
    velocity.y += 0.05f; // Gravity effect
    currentLife--;
    
    // Fade out
    color.a = (unsigned char)((float)currentLife / lifespan * 255.0f);
    
    return currentLife > 0;
}

void Particle::draw() {
    DrawCircleV(position, size, color);
}