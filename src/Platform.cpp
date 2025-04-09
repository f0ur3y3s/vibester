#include "Platform.h"

Platform::Platform(float x, float y, float width, float height, Color col) : 
    color(col) {
    rect = {x, y, width, height};
}

void Platform::draw() {
    DrawRectangleRec(rect, color);
}