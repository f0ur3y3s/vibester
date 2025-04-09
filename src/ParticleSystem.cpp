#include "ParticleSystem.h"
#include <cmath>

// Function to create splash particles
std::vector<Particle> createSplashParticles(Vector2 position, int count) {
    std::vector<Particle> particles;
    for (int i = 0; i < count; i++) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(2, 5);
        Vector2 velocity = {(float)cos(angle) * speed, (float)sin(angle) * speed};
        float size = (float)GetRandomValue(2, 6);
        int life = GetRandomValue(20, 40);
        
        Color color = SKYBLUE;
        color.a = 200;
        
        particles.push_back(Particle(position, velocity, size, life, color));
    }
    return particles;
}

// Function to create blast particles for death animation
std::vector<Particle> createBlastParticles(Vector2 position, int count, Color baseColor) {
    std::vector<Particle> particles;
    for (int i = 0; i < count; i++) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(5, 15);
        Vector2 velocity = {(float)cos(angle) * speed, (float)sin(angle) * speed};
        float size = (float)GetRandomValue(3, 8);
        int life = GetRandomValue(30, 60);
        
        // Vary color slightly
        Color color = baseColor;
        // Using custom clamp function instead of raylib's Clamp
        color.r = (unsigned char)clamp((float)color.r + GetRandomValue(-30, 30), 0.0f, 255.0f);
        color.g = (unsigned char)clamp((float)color.g + GetRandomValue(-30, 30), 0.0f, 255.0f);
        color.b = (unsigned char)clamp((float)color.b + GetRandomValue(-30, 30), 0.0f, 255.0f);
        color.a = 200;
        
        particles.push_back(Particle(position, velocity, size, life, color));
    }
    return particles;
}

// Helper function to clamp values
float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}