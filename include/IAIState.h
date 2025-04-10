// IAIState.h
#pragma once

#include <deque>
#include <vector>
#include <memory>
#include "StateManager.h"
#include "raylib.h" // For Rectangle type

class Character;
struct Vector2;

class IAIState {
public:
    virtual ~IAIState() = default;

    virtual void UpdateState(Character* enemy, Character* player, int frameCount) = 0;
    virtual void AnalyzePlayerPatterns() = 0;
    virtual bool DetectPlayerHabit(
        const std::deque<CharacterState>& history,
        CharacterState state,
        float threshold) = 0;
};
