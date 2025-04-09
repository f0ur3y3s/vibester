#include "AttackBox.h"

AttackBox::AttackBox(Rectangle r, int dmg, float kbx, float kby, int dur) : 
    rect(r), damage(dmg), knockbackX(kbx), knockbackY(kby), duration(dur), currentFrame(0) {}

bool AttackBox::update() {
    currentFrame++;
    return currentFrame < duration;
}

void AttackBox::draw() {
    // Debug visualization
    DrawRectangleLinesEx(rect, 2.0f, RED);
}