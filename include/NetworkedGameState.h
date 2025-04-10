// NetworkedGameState.h
#ifndef NETWORKED_GAME_STATE_H
#define NETWORKED_GAME_STATE_H

#include "GameState.h"
#include "NetworkManager.h"
#include <queue>
#include <deque>

// Extended GameState for network play
class NetworkedGameState : public GameState {
public:
    // Constructors
    NetworkedGameState();

    // Network-specific state
    enum NetworkGameMode {
        LOCAL_ONLY,      // Regular local play
        HOST,           // Server/host
        CLIENT         // Client
    };

    // Network mode functions
    void setNetworkMode(NetworkGameMode mode);
    NetworkGameMode getNetworkMode() const { return networkMode; }
    bool isNetworked() const { return networkMode != LOCAL_ONLY; }
    bool isNetworkHost() const { return networkMode == HOST; }

    // Connection management
    bool hostGame(int port = 7777);
    bool joinGame(const std::string& address, int port = 7777);
    void disconnectFromGame();

    // Matchmaking support
    bool createNetworkMatch(const std::string& matchName);
    bool joinNetworkMatch(const std::string& matchCode);
    std::vector<std::string> getAvailableMatches();

    // Override base class update with network capability
    virtual void update();

    // Override base class draw with network capability
    virtual void draw();

    // Process remote player input
    void processRemoteInput();

    // Send local player input
    void sendLocalInput();

    // Synchronize game state
    void synchronizeGameState();

    // Network latency management
    void setInputDelay(int frames) { inputDelayFrames = frames; }
    int getInputDelay() const { return inputDelayFrames; }
    int getAveragePing() const;

    // Chat system
    void sendChatMessage(const std::string& message);
    bool receiveChatMessage(std::string& message);
    std::vector<std::string> getChatHistory() const { return chatHistory; }

    // Spectator mode
    void enableSpectatorMode(bool enable) { spectatorMode = enable; }
    bool isSpectatorMode() const { return spectatorMode; }

    // Network options
    void setRollbackEnabled(bool enable) { rollbackEnabled = enable; }
    bool isRollbackEnabled() const { return rollbackEnabled; }

    // Sync & debug information
    int getFrameAdvantage() const { return frameAdvantage; }
    float getSyncPercentage() const { return syncPercentage; }

private:
    // Network mode of the game
    NetworkGameMode networkMode;

    // Network synchronization
    uint32_t networkFrame;
    std::deque<GameStatePacket> stateBuffer;
    std::deque<NetworkInput> inputBuffer;
    std::queue<NetworkInput> remoteInputQueue;

    // Input handling
    NetworkInput currentLocalInput;
    NetworkInput currentRemoteInput;
    std::deque<NetworkInput> localInputHistory;
    std::deque<NetworkInput> remoteInputHistory;

    // Latency management
    int inputDelayFrames;
    int frameAdvantage;
    float syncPercentage;

    // Chat system
    std::vector<std::string> chatHistory;
    bool newChatMessage;

    // Other flags
    bool spectatorMode;
    bool rollbackEnabled;

    // Helper methods
    void applyNetworkInput(Character* character, const NetworkInput& input);
    void captureLocalInput(NetworkInput& input);
    uint32_t calculateStateChecksum(const GameStatePacket& state);
    void constructGameStatePacket(GameStatePacket& packet);
};

#endif // NETWORKED_GAME_STATE_H