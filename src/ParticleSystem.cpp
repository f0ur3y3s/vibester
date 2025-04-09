#include "ParticleSystem.h"
#include <math.h>

// Helper function to clamp values
float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Create splash particles for effects like landing, hits, etc.
std::vector<Particle> createSplashParticles(Vector2 position, int count) {
    std::vector<Particle> particles;

    for (int i = 0; i < count; i++) {
        // Random velocity in all directions
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(2, 6);
        Vector2 velocity = {
            cosf(angle) * speed,
            sinf(angle) * speed
        };

        // Random size
        float size = GetRandomValue(2, 6);

        // Random lifespan
        int lifespan = GetRandomValue(20, 40);

        // Random color variations (white to light blue)
        Color color = {
            (unsigned char)GetRandomValue(200, 255),
            (unsigned char)GetRandomValue(200, 255),
            (unsigned char)GetRandomValue(230, 255),
            255
        };

        particles.push_back(Particle(position, velocity, size, lifespan, color));
    }

    return particles;
}

// Create blast particles for death animations or explosions
std::vector<Particle> createBlastParticles(Vector2 position, int count, Color baseColor) {
    std::vector<Particle> particles;

    for (int i = 0; i < count; i++) {
        // Random velocity in all directions but stronger
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(5, 12);
        Vector2 velocity = {
            cosf(angle) * speed,
            sinf(angle) * speed
        };

        // Larger random size
        float size = GetRandomValue(4, 10);

        // Longer lifespan
        int lifespan = GetRandomValue(30, 60);

        // Color variations based on the input color
        Color color = {
            (unsigned char)clamp(baseColor.r + GetRandomValue(-20, 20), 0, 255),
            (unsigned char)clamp(baseColor.g + GetRandomValue(-20, 20), 0, 255),
            (unsigned char)clamp(baseColor.b + GetRandomValue(-20, 20), 0, 255),
            255
        };

        particles.push_back(Particle(position, velocity, size, lifespan, color));
    }

    return particles;
}