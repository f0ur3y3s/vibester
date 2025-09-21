#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "raylib.h"
#include "Particle.h"
#include <vector>
#include <cmath>
#include <algorithm>

// Helper function to clamp values - declare first so it's available to all functions
inline int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Function prototypes
std::vector<Particle> createSplashParticles(Vector2 position, int count);
std::vector<Particle> createBlastParticles(Vector2 position, int count, Color baseColor);
std::vector<Particle> createMassiveExplosionParticles(Vector2 position, int count, Color baseColor);
std::vector<Particle> createExplosionParticles(Vector2 position, int count, Color baseColor);
std::vector<Particle> createHitParticles(Vector2 position, Vector2 direction, int count, Color color);
bool updateParticles(std::vector<Particle>& particles);
void drawParticles(const std::vector<Particle>& particles);

#endif // PARTICLE_SYSTEM_H