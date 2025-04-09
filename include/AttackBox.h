#ifndef ATTACKBOX_H
#define ATTACKBOX_H

#include "raylib.h"

// Attack hitbox
struct AttackBox {
    Rectangle rect;
    int damage;
    float knockbackX;
    float knockbackY;
    int duration;
    int currentFrame;
    
    AttackBox(Rectangle r, int dmg, float kbx, float kby, int dur);
    
    bool update();
    void draw();
};

#endif // ATTACKBOX_H