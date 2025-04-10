// GameStateObserver.h
#pragma once

#include <vector>

class Character;
class Platform;

class GameStateObserver {
public:
    virtual ~GameStateObserver() = default;

    virtual void OnGameStateUpdated(std::vector<Character*>& players,
                                   const std::vector<Platform>& platforms) = 0;
};
