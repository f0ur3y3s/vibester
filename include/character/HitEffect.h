#ifndef HIT_EFFECT_H
#define HIT_EFFECT_H

#include "raylib.h"

// Visual hit effect displayed when attacks connect
class HitEffect {
public:
    Vector2 position;
    Color color;
    int duration;
    int currentFrame;
    float size;

    HitEffect(Vector2 pos, Color col);
    bool update();
    void draw();
};

#endif // HIT_EFFECT_H