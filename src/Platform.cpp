#include "Platform.h"

#include <cmath>

Platform::Platform(float x, float y, float width, float height, Color col) {
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    color = col;
}

void Platform::draw() {
    DrawRectangleRec(rect, color);

    // Draw platform edge highlight
    Color highlightColor = {
        (unsigned char)fmin(color.r + 40, 255),
        (unsigned char)fmin(color.g + 40, 255),
        (unsigned char)fmin(color.b + 40, 255),
        color.a
    };
    DrawRectangleLines(rect.x, rect.y, rect.width, rect.height, highlightColor);

    // Draw top edge with brighter color to make it more visible
    DrawLine(
        rect.x, rect.y,
        rect.x + rect.width, rect.y,
        highlightColor
    );
}