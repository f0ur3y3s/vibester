#include "ParticleSystem.h"
#include <math.h>

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

// Create explosion particles
std::vector<Particle> createExplosionParticles(Vector2 position, int count, Color baseColor) {
    std::vector<Particle> particles;
    particles.reserve(count);

    for (int i = 0; i < count; i++) {
        // Random angle and speed
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(2, 8);
        Vector2 velocity = {
            cosf(angle) * speed,
            sinf(angle) * speed
        };

        // Random particle properties
        float size = GetRandomValue(2, 6);
        int lifespan = GetRandomValue(15, 45);

        // Slightly vary the color
        Color particleColor = baseColor;
        particleColor.r = (unsigned char)clamp(particleColor.r + GetRandomValue(-20, 20), 0, 255);
        particleColor.g = (unsigned char)clamp(particleColor.g + GetRandomValue(-20, 20), 0, 255);
        particleColor.b = (unsigned char)clamp(particleColor.b + GetRandomValue(-20, 20), 0, 255);

        particles.push_back(Particle(position, velocity, size, lifespan, particleColor));
    }

    return particles;
}

// Create hit particles
std::vector<Particle> createHitParticles(Vector2 position, Vector2 direction, int count, Color color) {
    std::vector<Particle> particles;
    particles.reserve(count);

    float baseAngle = atan2f(direction.y, direction.x);

    for (int i = 0; i < count; i++) {
        // Particles spread in the general direction of the hit
        float angleSpread = GetRandomValue(-30, 30) * DEG2RAD;
        float angle = baseAngle + angleSpread;
        float speed = GetRandomValue(3, 8);

        Vector2 velocity = {
            cosf(angle) * speed,
            sinf(angle) * speed
        };

        float size = GetRandomValue(2, 5);
        int lifespan = GetRandomValue(10, 25);

        particles.push_back(Particle(position, velocity, size, lifespan, color));
    }

    return particles;
}

std::vector<Particle> createMassiveExplosionParticles(Vector2 position, int count, Color baseColor) {
    std::vector<Particle> particles;

    // Create core explosion particles
    for (int i = 0; i < count; i++) {
        // Random velocity in all directions but with high speed
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(8, 20);  // Higher speed than regular explosions
        Vector2 velocity = {
            cosf(angle) * speed,
            sinf(angle) * speed
        };

        // Larger random size
        float size = GetRandomValue(5, 15);  // Bigger particles

        // Longer lifespan
        int lifespan = GetRandomValue(40, 100);  // Particles last longer

        // Color variations based on the input color
        Color color;

        // Mix of fire colors and character colors
        int colorType = GetRandomValue(0, 10);
        if (colorType < 3) {
            // Use character color with variations
            color = {
                (unsigned char)clamp(baseColor.r + GetRandomValue(-20, 20), 0, 255),
                (unsigned char)clamp(baseColor.g + GetRandomValue(-20, 20), 0, 255),
                (unsigned char)clamp(baseColor.b + GetRandomValue(-20, 20), 0, 255),
                255
            };
        } else if (colorType < 7) {
            // Fire colors (red, orange, yellow)
            int firePalette = GetRandomValue(0, 2);
            switch (firePalette) {
                case 0: color = RED; break;
                case 1: color = ORANGE; break;
                case 2: color = YELLOW; break;
            }
        } else {
            // White/smoke particles
            int grayscale = GetRandomValue(180, 255);
            color = {
                (unsigned char)grayscale,
                (unsigned char)grayscale,
                (unsigned char)grayscale,
                255
            };
        }

        particles.push_back(Particle(position, velocity, size, lifespan, color));
    }

    // Add spark particles
    int sparkCount = count / 4;
    for (int i = 0; i < sparkCount; i++) {
        // Random velocity in all directions but with higher speed
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(15, 30);  // Spark particles move faster
        Vector2 velocity = {
            cosf(angle) * speed,
            sinf(angle) * speed
        };

        // Smaller spark size
        float size = GetRandomValue(1, 3);  // Small sparks

        // Short lifespan
        int lifespan = GetRandomValue(10, 30);  // Short-lived sparks

        // Bright colors for sparks
        Color color;
        int sparkColor = GetRandomValue(0, 2);
        switch (sparkColor) {
            case 0: color = YELLOW; break;
            case 1: color = WHITE; break;
            case 2: color = (Color){255, 200, 50, 255}; // Bright orange
        }

        particles.push_back(Particle(position, velocity, size, lifespan, color));
    }

    // Add debris particles
    int debrisCount = count / 5;
    for (int i = 0; i < debrisCount; i++) {
        // Random velocity in all directions with medium speed
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(5, 12);
        Vector2 velocity = {
            cosf(angle) * speed,
            sinf(angle) * speed
        };

        // Medium size debris
        float size = GetRandomValue(3, 8);

        // Medium lifespan
        int lifespan = GetRandomValue(30, 70);

        // Dark colors for debris
        Color color;
        int debrisColor = GetRandomValue(0, 3);
        switch (debrisColor) {
            case 0: color = DARKGRAY; break;
            case 1: color = BLACK; break;
            case 2: color = (Color){50, 50, 50, 255}; // Very dark gray
            case 3: color = baseColor; // Character original color
        }

        particles.push_back(Particle(position, velocity, size, lifespan, color));
    }

    return particles;
}

// Function to update particles
bool updateParticles(std::vector<Particle>& particles) {
    for (int i = 0; i < particles.size(); i++) {
        if (!particles[i].update()) {
            // Remove dead particles
            particles.erase(particles.begin() + i);
            i--;
        }
    }

    return !particles.empty();
}

// Function to draw particles
void drawParticles(const std::vector<Particle>& particles) {
    for (int i = 0; i < particles.size(); i++) {
        // Create a non-const copy to work around the const issue
        Particle p = particles[i];
        p.draw();
    }
}