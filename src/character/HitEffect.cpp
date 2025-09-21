#include "../../include/character/HitEffect.h"

HitEffect::HitEffect(Vector2 pos, Color col)
{
    position = pos;
    color = col;
    duration = 10;
    currentFrame = 0;
    size = 20.0f;
}

bool HitEffect::update()
{
    currentFrame++;
    return currentFrame < duration;
}

void HitEffect::draw()
{
    float scale = 1.0f - static_cast<float>(currentFrame) / duration;
    float alpha = 255.0f * scale;
    
    Color effectColor = color;
    effectColor.a = static_cast<unsigned char>(alpha);
    
    DrawCircleV(position, size * scale, effectColor);
    DrawCircleLines(position.x, position.y, size * scale * 1.2f, effectColor);
}