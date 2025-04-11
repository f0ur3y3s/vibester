// NetworkManager.h
#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "raylib.h"
#include "Character.h"
#include "GameState.h"
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

// Define network message types
enum NetworkMessageType {
    MSG_CONNECT_REQUEST = 1,
    MSG_CONNECT_ACCEPT,
    MSG_CONNECT_DENY,
    MSG_DISCONNECT,
    MSG_GAME_START,
    MSG_GAME_START_ACK,
    MSG_GAME_END,
    MSG_INPUT_UPDATE,
    MSG_GAME_STATE_UPDATE,
    MSG_PING,
    MSG_PONG,
    MSG_CHAT
};

// Define player input state for networking
struct NetworkInput {
    uint32_t frame;          // Frame number this input was recorded on

    // Movement flags
    bool moveLeft;
    bool moveRight;
    bool jump;
    bool fastFall;

    // Direction indicators
    bool up;
    bool down;

    // Attack flags
    bool attack;
    bool special;
    bool smashAttack;
    bool grab;

    // Defensive flags
    bool shield;
    bool spot_dodge;
    bool forward_dodge;
    bool backward_dodge;

    // NOTE: We removed the dropThrough field as it was causing compilation errors
    // Instead, we handle platform drop-through using a combination of existing flags
};

// Define game state packet for synchronization
struct GameStatePacket {
    uint32_t frame;
    uint32_t checksum;  // For validation
    uint32_t extraData; // Extra data for game state sync
    struct PlayerState {
        Vector2 position;
        Vector2 velocity;
        float damagePercent;
        int stocks;
        int stateID;
        bool isFacingRight;
        bool isAttacking;
        int currentAttack;
        int attackFrame;
    } players[2];
};

// Connection info for peer-to-peer or client-server model
struct PeerInfo {
    std::string address;
    int port;
    bool isConnected;
    int playerID;
    std::string playerName;
    int ping;
    uint64_t lastPingTime;
};

class NetworkManager {
public:
    // Singleton instance
    static NetworkManager& getInstance();

    // Initialize networking
    bool initialize();
    void shutdown();

    // Connection management
    bool startServer(int port);
    bool connectToServer(const std::string& address, int port);
    void disconnect();
    bool sendToAll(const void* data, int size);

    // Network mode
    enum NetworkMode {
        OFFLINE,
        SERVER,
        CLIENT
    };
    NetworkMode getMode() const { return mode; }

    // Session management
    bool isHost() const { return mode == SERVER; }
    int getPlayerID() const { return localPlayerID; }
    void handlePong(const uint8_t* data, int size);
    bool isConnected() const;
    int getAveragePing() const;
    std::string getLocalPlayerName() const;
    void setLocalPlayerName(const std::string& name);


    // Matchmaking
    bool createMatch(const std::string& matchName);
    bool joinMatch(const std::string& matchCode);
    void leaveMatch();
    std::vector<std::string> getAvailableMatches();

    // Input and state management
    void sendInput(const NetworkInput& input);
    void sendGameState(const GameStatePacket& state);
    bool getRemoteInput(NetworkInput& input);
    bool getRemoteGameState(GameStatePacket& state);

    // Update network (call every frame)
    void update();

    // Chat functionality
    void sendChatMessage(const std::string& message);
    bool receiveChatMessage(std::string& message);
    
    // Game control messages
    bool hasGameStartMessage();

    // Network configuration
    static constexpr int BUFFER_SIZE = 2048;
    static constexpr int DEFAULT_PORT = 7777;
    static constexpr int PING_INTERVAL_MS = 1000;
    static constexpr int TIMEOUT_MS = 5000;
    static constexpr int MAX_PLAYERS = 8;

    // Remote peers
    std::vector<PeerInfo> peers;

private:
    // Private constructor for singleton
    NetworkManager();
    ~NetworkManager();

    // Disable copy/move
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    NetworkManager(NetworkManager&&) = delete;
    NetworkManager& operator=(NetworkManager&&) = delete;

    // Network socket handling
    void handleIncomingMessages();
    bool sendMessage(const void* data, int size, const std::string& address, int port);

    // Network protocol implementation
    void handleConnectRequest(const std::string& senderAddr, int senderPort, const uint8_t* data, int size);
    void handleConnectAccept(const uint8_t* data, int size);
    void handleDisconnect(const uint8_t* data, int size);
    void handleInputUpdate(const uint8_t* data, int size);
    void handleGameStateUpdate(const uint8_t* data, int size);
    void handlePing(const std::string& senderAddr, int senderPort, const uint8_t* data, int size);

    // Packet construction helpers
    std::vector<uint8_t> createConnectRequestPacket();
    std::vector<uint8_t> createGameStatePacket(const GameStatePacket& state);
    std::vector<uint8_t> createInputPacket(const NetworkInput& input);

    // Networking state
    NetworkMode mode;
    bool initialized;
    bool connected;
    int socketHandle;
    int localPlayerID;
    std::string localPlayerName;
    std::string serverAddress;
    int serverPort;

    // Threading for network operations
    std::thread networkThread;
    std::atomic<bool> running;

    // Input and state queues
    std::queue<NetworkInput> inputQueue;
    std::queue<GameStatePacket> stateQueue;
    std::queue<std::string> chatQueue;
    std::mutex queueMutex;
    
    // Game control flags
    std::atomic<bool> gameStartReceived;

    // Statistics
    int averagePing;
    uint32_t lastFrameSent;
};

#endif // NETWORK_MANAGER_H