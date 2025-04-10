#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include "raylib.h"
#include "Particle.h"
#include <vector>

// Function to create splash particles
std::vector<Particle> createSplashParticles(Vector2 position, int count);

// Function to create blast particles for death animation
std::vector<Particle> createBlastParticles(Vector2 position, int count, Color baseColor);

std::vector<Particle> createMassiveExplosionParticles(Vector2 position, int count, Color baseColor);

// Helper function to clamp values
float clamp(float value, float min, float max);

#endif // PARTICLESYSTEM_H