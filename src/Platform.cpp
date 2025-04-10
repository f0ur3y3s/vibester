#include "Platform.h"
#include <algorithm> // Include it here as well for safety

// Constructor implementation matching header declaration
Platform::Platform(float x, float y, float width, float height, Color col, PlatformType platformType) {
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    color = col;
    type = platformType;
}

void Platform::draw() {
    DrawRectangleRec(rect, color);

    // Draw platform edge highlight
    Color highlightColor = {
        (unsigned char)std::min(color.r + 40, 255),
        (unsigned char)std::min(color.g + 40, 255),
        (unsigned char)std::min(color.b + 40, 255),
        color.a
    };

    // Different visual styles for different platform types
    if (type == PASSTHROUGH) {
        // For pass-through platforms, only highlight the top edge
        DrawLine(
            rect.x, rect.y,
            rect.x + rect.width, rect.y,
            highlightColor
        );
    } else {
        // For solid platforms, highlight all edges
        DrawRectangleLines(rect.x, rect.y, rect.width, rect.height, highlightColor);
    }
}