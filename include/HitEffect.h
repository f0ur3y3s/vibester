#ifndef HIT_EFFECT_H
#define HIT_EFFECT_H

#include "raylib.h"
#include <cmath>

// Hit effect implementation
HitEffect::HitEffect(Vector2 pos, Color col) {
    position = pos;
    color = col;
    duration = 15;
    currentFrame = 0;
    size = 30.0f;
}

bool HitEffect::update() {
    currentFrame++;
    size -= 1.5f;
    return currentFrame < duration;
}

void HitEffect::draw() {
    float alpha = 1.0f - (float)currentFrame / duration;
    Color effectColor = {color.r, color.g, color.b, (unsigned char)(255 * alpha)};
    DrawCircleV(position, size, effectColor);

    // Draw impact lines
    for (int i = 0; i < 8; i++) {
        float angle = i * 45.0f * DEG2RAD;
        float lineLength = size * 1.5f * (1.0f - (float)currentFrame / duration);
        Vector2 end = {
            position.x + static_cast<float>(cos(angle) * lineLength),
            position.y + static_cast<float>(sin(angle) * lineLength)
        };
        DrawLineEx(position, end, 3.0f, effectColor);
    }
}

#endif // HIT_EFFECT_H