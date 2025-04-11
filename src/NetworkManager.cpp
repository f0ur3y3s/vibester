#include "NetworkManager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <vector>
#include <string>

// Network-specific headers
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SocketHandle;
    #define INVALID_SOCKET_HANDLE INVALID_SOCKET
    #define SOCKET_ERROR_CODE SOCKET_ERROR
    #define CLOSE_SOCKET(s) closesocket(s)
    #define GET_SOCKET_ERROR() WSAGetLastError()
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <netdb.h>
    typedef int SocketHandle;
    #define INVALID_SOCKET_HANDLE -1
    #define SOCKET_ERROR_CODE -1
    #define CLOSE_SOCKET(s) close(s)
    #define GET_SOCKET_ERROR() errno
#endif

// Singleton instance
NetworkManager& NetworkManager::getInstance() {
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager()
    : mode(OFFLINE)
    , initialized(false)
    , connected(false)
    , socketHandle(INVALID_SOCKET_HANDLE)
    , localPlayerID(0)
    , localPlayerName("Player")
    , serverPort(DEFAULT_PORT)
    , running(false)
    , gameStartReceived(false)
    , averagePing(0)
    , lastFrameSent(0)
{
}

NetworkManager::~NetworkManager() {
    shutdown();
}

// Initialize the network manager
bool NetworkManager::initialize() {
    if (initialized) {
        return true;
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return false;
    }
#endif

    // Create UDP socket
    socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketHandle == INVALID_SOCKET_HANDLE) {
        std::cerr << "Failed to create socket: " << GET_SOCKET_ERROR() << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    // Set socket to non-blocking mode
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(socketHandle, FIONBIO, &mode) != 0) {
        std::cerr << "Failed to set non-blocking mode: " << GET_SOCKET_ERROR() << std::endl;
        CLOSE_SOCKET(socketHandle);
        WSACleanup();
        return false;
    }
#else
    int flags = fcntl(socketHandle, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get socket flags: " << GET_SOCKET_ERROR() << std::endl;
        CLOSE_SOCKET(socketHandle);
        return false;
    }

    if (fcntl(socketHandle, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set non-blocking mode: " << GET_SOCKET_ERROR() << std::endl;
        CLOSE_SOCKET(socketHandle);
        return false;
    }
#endif

    // Increase socket buffer sizes for better performance
    int recvBufSize = 1024 * 1024; // 1MB
    int sendBufSize = 1024 * 1024; // 1MB

    if (setsockopt(socketHandle, SOL_SOCKET, SO_RCVBUF, (char*)&recvBufSize, sizeof(recvBufSize)) == -1) {
        std::cerr << "Warning: Failed to set receive buffer size: " << GET_SOCKET_ERROR() << std::endl;
        // Non-fatal, continue
    }

    if (setsockopt(socketHandle, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sizeof(sendBufSize)) == -1) {
        std::cerr << "Warning: Failed to set send buffer size: " << GET_SOCKET_ERROR() << std::endl;
        // Non-fatal, continue
    }

    initialized = true;
    std::cout << "NetworkManager successfully initialized" << std::endl;
    return true;
}

// Periodic update for ping, timeouts, etc.
void NetworkManager::update() {
    if (!connected) {
        return;
    }

    // Send ping to all peers periodically
    static uint64_t lastPingTime = 0;
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    if (currentTime - lastPingTime > PING_INTERVAL_MS) {
        uint8_t pingMsg[9];
        pingMsg[0] = MSG_PING;
        *reinterpret_cast<uint32_t*>(&pingMsg[1]) = localPlayerID;
        *reinterpret_cast<uint32_t*>(&pingMsg[5]) = static_cast<uint32_t>(currentTime);

        if (mode == CLIENT) {
            sendMessage(pingMsg, sizeof(pingMsg), serverAddress, serverPort);
        } else if (mode == SERVER) {
            sendToAll(pingMsg, sizeof(pingMsg));
        }

        lastPingTime = currentTime;
    }

    // Check for disconnected peers (timeout)
    for (auto it = peers.begin(); it != peers.end();) {
        if (currentTime - it->lastPingTime > TIMEOUT_MS) {
            std::cout << "Peer " << it->playerName << " (ID: " << it->playerID << ") timed out" << std::endl;
            it = peers.erase(it);
        } else {
            ++it;
        }
    }
}

// Send message to all connected peers
bool NetworkManager::sendToAll(const void* data, int size) {
    if (mode != SERVER) {
        std::cout << "NetworkManager: sendToAll failed - not in server mode" << std::endl;
        return false;
    }

    // Debug the message being sent
    if (size > 0) {
        uint8_t msgType = *reinterpret_cast<const uint8_t*>(data);
        if (msgType != MSG_PING && msgType != MSG_PONG && msgType != MSG_INPUT_UPDATE) {
            std::cout << "NetworkManager: sendToAll - Sending message type " << (int)msgType
                    << " to " << peers.size() << " peers" << std::endl;
        }

        // Special handling for game start message
        if (msgType == MSG_GAME_START) {
            std::cout << "NetworkManager: Broadcasting MSG_GAME_START to all clients" << std::endl;

            // For game start, send multiple times with small delays to ensure delivery
            bool success = true;
            for (int retry = 0; retry < 3; retry++) {
                int sentCount = 0;
                for (const auto& peer : peers) {
                    if (sendMessage(data, size, peer.address, peer.port)) {
                        sentCount++;
                    } else {
                        success = false;
                    }
                }

                // Small delay between retries
                if (retry < 2) std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            return success;
        }
    }

    bool success = true;
    int sentCount = 0;

    for (const auto& peer : peers) {
        if (sendMessage(data, size, peer.address, peer.port)) {
            sentCount++;
        } else {
            success = false;
        }
    }

    return success;
}

// Send message to remote endpoint
bool NetworkManager::sendMessage(const void* data, int size, const std::string& address, int port) {
    if (!initialized) {
        std::cerr << "Cannot send: Network manager not initialized" << std::endl;
        return false;
    }

    // Log the message unless it's a high-frequency type
    uint8_t msgType = *reinterpret_cast<const uint8_t*>(data);
    if (msgType != MSG_PING && msgType != MSG_PONG && msgType != MSG_INPUT_UPDATE) {
        std::cout << "Sending message type " << (int)msgType << " to " << address << ":" << port
                  << " (" << size << " bytes)" << std::endl;
    }

    sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_addr.s_addr = inet_addr(address.c_str());
    destAddr.sin_port = htons(port);

#ifdef _WIN32
    int bytesSent = sendto(socketHandle, reinterpret_cast<const char*>(data), size, 0,
                           reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));
#else
    int bytesSent = sendto(socketHandle, data, size, 0,
                           reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr));
#endif

    if (bytesSent == SOCKET_ERROR_CODE) {
        std::cerr << "Send failed to " << address << ":" << port << " - Error: " << GET_SOCKET_ERROR() << std::endl;
        return false;
    }

    return true;
}

// Create a connection request packet
std::vector<uint8_t> NetworkManager::createConnectRequestPacket() {
    std::vector<uint8_t> packet;
    packet.push_back(MSG_CONNECT_REQUEST);

    // Add player name length
    uint32_t nameLength = static_cast<uint32_t>(localPlayerName.length());
    packet.insert(packet.end(),
                  reinterpret_cast<uint8_t*>(&nameLength),
                  reinterpret_cast<uint8_t*>(&nameLength) + sizeof(nameLength));

    // Add player name
    packet.insert(packet.end(), localPlayerName.begin(), localPlayerName.end());

    return packet;
}

// Create packet for input update
std::vector<uint8_t> NetworkManager::createInputPacket(const NetworkInput& input) {
    std::vector<uint8_t> packet;
    packet.push_back(MSG_INPUT_UPDATE);

    // Add local player ID
    packet.insert(packet.end(),
                  reinterpret_cast<const uint8_t*>(&localPlayerID),
                  reinterpret_cast<const uint8_t*>(&localPlayerID) + sizeof(localPlayerID));

    // Add input data
    packet.insert(packet.end(),
                  reinterpret_cast<const uint8_t*>(&input),
                  reinterpret_cast<const uint8_t*>(&input) + sizeof(NetworkInput));

    return packet;
}

// Create packet for game state update
std::vector<uint8_t> NetworkManager::createGameStatePacket(const GameStatePacket& state) {
    std::vector<uint8_t> packet;
    packet.push_back(MSG_GAME_STATE_UPDATE);

    // Add game state data
    packet.insert(packet.end(),
                  reinterpret_cast<const uint8_t*>(&state),
                  reinterpret_cast<const uint8_t*>(&state) + sizeof(GameStatePacket));

    return packet;
}

// Check if game start message was received
bool NetworkManager::hasGameStartMessage() {
    // Check the flag atomically
    bool expected = true;
    bool desired = false;

    // Only reset the flag if it was actually set to true
    if (gameStartReceived.compare_exchange_strong(expected, desired)) {
        std::cout << "NetworkManager: Game start message detected and flag reset" << std::endl;
        return true;
    }

    return false;
}

// Set local player name
void NetworkManager::setLocalPlayerName(const std::string& name) {
    localPlayerName = name;
}

// Get local player name
std::string NetworkManager::getLocalPlayerName() const {
    return localPlayerName;
}

// Get average ping time
int NetworkManager::getAveragePing() const {
    return averagePing;
}

// Check if connected
bool NetworkManager::isConnected() const {
    return connected;
}

void NetworkManager::shutdown() {
    if (!initialized) {
        return;
    }

    // Stop network thread
    if (running.load()) {
        running.store(false);
        if (networkThread.joinable()) {
            networkThread.join();
        }
    }

    // Disconnect if connected
    if (connected) {
        disconnect();
    }

    // Close socket
    if (socketHandle != INVALID_SOCKET_HANDLE) {
        CLOSE_SOCKET(socketHandle);
        socketHandle = INVALID_SOCKET_HANDLE;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    initialized = false;
    mode = OFFLINE;
}

// Start the server listening for connections
bool NetworkManager::startServer(int port) {
    if (!initialized) {
        std::cerr << "Failed to start server: Network manager not initialized" << std::endl;
        return false;
    }

    serverPort = port;
    std::cout << "Starting server on port " << port << std::endl;

    // Bind socket to the specified port
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(serverPort);

    if (bind(socketHandle, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR_CODE) {
        std::cerr << "Failed to bind socket: " << GET_SOCKET_ERROR() << std::endl;
        return false;
    }

    // Display hostname and local interfaces for easier connection setup
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    std::cout << "Server hostname: " << hostname << std::endl;
    std::cout << "Local IP addresses (use these for LAN connections):" << std::endl;

    // Print local IP addresses
#ifdef _WIN32
    struct hostent* host = gethostbyname(hostname);
    if (host) {
        for (int i = 0; host->h_addr_list[i] != 0; ++i) {
            struct in_addr addr;
            memcpy(&addr, host->h_addr_list[i], sizeof(struct in_addr));
            std::cout << "  Interface " << i << ": " << inet_ntoa(addr) << std::endl;
        }
    }
#else
    std::cout << "  127.0.0.1 (local loopback)" << std::endl;
    // For a more complete solution, you'd want to iterate through network interfaces here
#endif

    // Set up server state
    mode = SERVER;
    connected = true;
    localPlayerID = 0;  // Server is always player 0

    // Start network thread
    running.store(true);
    networkThread = std::thread(&NetworkManager::handleIncomingMessages, this);

    std::cout << "Server started and waiting for connections on port " << serverPort << std::endl;
    std::cout << "Also listening on 127.0.0.1 for local connections" << std::endl;
    return true;
}

bool NetworkManager::connectToServer(const std::string& address, int port) {
    if (!initialized) {
        std::cerr << "Failed to connect: Network manager not initialized" << std::endl;
        return false;
    }

    std::cout << "Attempting to connect to " << address << ":" << port << std::endl;
    serverAddress = address;
    serverPort = port;

    // Set up client state
    mode = CLIENT;

    // Send connect request to server with retry logic
    std::vector<uint8_t> connectPacket = createConnectRequestPacket();
    bool sentSuccessfully = false;

    // Try to send connection request multiple times
    for (int attempt = 0; attempt < 5; attempt++) {
        if (sendMessage(connectPacket.data(), connectPacket.size(), serverAddress, serverPort)) {
            sentSuccessfully = true;
            std::cout << "Connect request sent successfully (attempt " << attempt+1 << ")" << std::endl;
            break;
        }
        std::cerr << "Failed to send connect request (attempt " << attempt+1 << ")" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    if (!sentSuccessfully) {
        std::cerr << "Failed to send any connect requests" << std::endl;
        mode = OFFLINE;
        return false;
    }

    // Start network thread to handle responses
    running.store(true);
    networkThread = std::thread(&NetworkManager::handleIncomingMessages, this);

    // Wait for connect response with timeout
    auto startTime = std::chrono::steady_clock::now();
    const int CONNECTION_TIMEOUT_MS = 8000; // 8 seconds

    while (!connected) {
        // Resend the connection request periodically
        static auto lastResendTime = startTime;
        auto currentTime = std::chrono::steady_clock::now();

        // Resend the connection request every 1 second
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - lastResendTime).count() >= 1000) {

            sendMessage(connectPacket.data(), connectPacket.size(), serverAddress, serverPort);
            std::cout << "Resending connection request..." << std::endl;
            lastResendTime = currentTime;
        }

        // Update waiting indicator
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - startTime).count();
        if (elapsed % 1000 < 20) {
            int dots = (elapsed / 1000) % 4;
            std::string waitMsg = "Waiting for server response";
            for (int i = 0; i < dots; i++) {
                waitMsg += ".";
            }
            std::cout << "\r" << waitMsg << std::string(10, ' ') << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Check for timeout
        if (elapsed > CONNECTION_TIMEOUT_MS) {
            std::cerr << "\nConnection to server timed out after " << elapsed << "ms" << std::endl;
            running.store(false);
            if (networkThread.joinable()) {
                networkThread.join();
            }
            mode = OFFLINE;
            return false;
        }
    }

    std::cout << "\nConnected to server at " << serverAddress << ":" << serverPort << std::endl;
    return true;
}

void NetworkManager::disconnect() {
    // Disconnect from network
    if (!connected) {
        return;
    }

    // Send disconnect message
    uint8_t disconnectMsg[5];
    disconnectMsg[0] = MSG_DISCONNECT;
    *reinterpret_cast<uint32_t*>(&disconnectMsg[1]) = localPlayerID;

    if (mode == CLIENT) {
        sendMessage(disconnectMsg, sizeof(disconnectMsg), serverAddress, serverPort);
    } else if (mode == SERVER) {
        sendToAll(disconnectMsg, sizeof(disconnectMsg));
    }

    // Clear all peers
    peers.clear();

    // Reset state
    connected = false;
    mode = OFFLINE;

    // Stop network thread
    running.store(false);
    if (networkThread.joinable()) {
        networkThread.join();
    }
}

bool NetworkManager::createMatch(const std::string& matchName) {
    if (mode != OFFLINE) {
        return false;
    }

    // In a real implementation, this would communicate with a matchmaking server
    // For this implementation, we just start a local server
    bool success = startServer(DEFAULT_PORT);

    if (success) {
        // Add match metadata
        std::cout << "Created match: " << matchName << std::endl;
    }

    return success;
}

bool NetworkManager::joinMatch(const std::string& matchCode) {
    if (mode != OFFLINE) {
        return false;
    }

    // Assume matchCode is the server IP address
    return connectToServer(matchCode, DEFAULT_PORT);
}

void NetworkManager::leaveMatch() {
    disconnect();
}

std::vector<std::string> NetworkManager::getAvailableMatches() {
    // In a real implementation, this would query a matchmaking server
    // For this implementation, we return a placeholder
    return {"No matches available via automatic discovery. Use direct IP connection."};
}

// Process incoming messages
void NetworkManager::handleIncomingMessages() {
    uint8_t buffer[BUFFER_SIZE];
    sockaddr_in senderAddr;
    socklen_t senderAddrLen = sizeof(senderAddr);

    std::cout << "Network thread started, listening for incoming messages" << std::endl;

    while (running.load()) {
        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);

        // Receive data
#ifdef _WIN32
        int bytesReceived = recvfrom(socketHandle, reinterpret_cast<char*>(buffer), BUFFER_SIZE, 0,
                                     reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrLen);
#else
        int bytesReceived = recvfrom(socketHandle, buffer, BUFFER_SIZE, 0,
                                     reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrLen);
#endif

        if (bytesReceived <= 0) {
            // No data available or error
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // Get sender info
        std::string senderIP = inet_ntoa(senderAddr.sin_addr);
        int senderPort = ntohs(senderAddr.sin_port);

        // Log the message
        std::cout << "Received " << bytesReceived << " bytes from " << senderIP << ":"
                  << senderPort << " [Type: " << (int)buffer[0] << "]" << std::endl;

        // Process message based on type
        uint8_t msgType = buffer[0];

        switch (msgType) {
            case MSG_CONNECT_REQUEST:
                std::cout << "Received connection request from " << senderIP << ":" << senderPort << std::endl;
                handleConnectRequest(senderIP, senderPort, buffer, bytesReceived);
                break;

            case MSG_CONNECT_ACCEPT:
                std::cout << "Received connection acceptance from server" << std::endl;
                handleConnectAccept(buffer, bytesReceived);
                break;

            case MSG_CONNECT_DENY:
                // Handle connection denied
                if (mode == CLIENT) {
                    std::cerr << "Connection denied by server" << std::endl;
                    connected = false;
                    mode = OFFLINE;
                    running.store(false);
                }
                break;

            case MSG_DISCONNECT:
                handleDisconnect(buffer, bytesReceived);
                break;

            case MSG_PING:
                // Handle ping
                handlePing(senderIP, senderPort, buffer, bytesReceived);
                break;

            case MSG_PONG:
                // Handle pong
                handlePong(buffer, bytesReceived);
                break;

            case MSG_GAME_START:
                if (mode == CLIENT) {
                    std::cout << "Game start message received from host!" << std::endl;
                    gameStartReceived.store(true);

                    // Send acknowledgment
                    uint8_t ackMsg[1] = { MSG_GAME_START_ACK };
                    sendMessage(ackMsg, sizeof(ackMsg), serverAddress, serverPort);
                    std::cout << "Sent game start acknowledgment to server" << std::endl;
                }
                break;

            case MSG_GAME_START_ACK:
                if (mode == SERVER) {
                    std::cout << "Client acknowledged game start" << std::endl;
                }
                break;

            case MSG_INPUT_UPDATE:
                handleInputUpdate(buffer, bytesReceived);
                break;

            case MSG_GAME_STATE_UPDATE:
                handleGameStateUpdate(buffer, bytesReceived);
                break;

            default:
                std::cout << "Unknown message type: " << (int)msgType << std::endl;
                break;
        }
    }

    std::cout << "Network thread exiting" << std::endl;
}

// Handle connection request (server side)
void NetworkManager::handleConnectRequest(const std::string& senderAddr, int senderPort,
                                        const uint8_t* data, int size) {
    if (mode != SERVER) return;

    std::cout << "Processing connection request from " << senderAddr << ":" << senderPort << std::endl;

    // Extract player name if available
    std::string playerName = "Player";
    if (size > 5) {
        uint32_t nameLength = *reinterpret_cast<const uint32_t*>(&data[1]);
        if (size >= 5 + nameLength && nameLength < 100) {
            playerName = std::string(reinterpret_cast<const char*>(&data[5]), nameLength);
        }
    }

    // Check if peer already exists (might be a duplicate request)
    for (const auto& peer : peers) {
        if (peer.address == senderAddr && peer.port == senderPort) {
            std::cout << "Peer already connected, sending acceptance again" << std::endl;

            // Send accept message again
            uint32_t playerIDCopy = peer.playerID;

            std::vector<uint8_t> response;
            response.push_back(MSG_CONNECT_ACCEPT);
            response.insert(response.end(),
                          reinterpret_cast<uint8_t*>(&playerIDCopy),
                          reinterpret_cast<uint8_t*>(&playerIDCopy) + sizeof(playerIDCopy));

            // Add server name length
            uint32_t nameLength = static_cast<uint32_t>(localPlayerName.length());
            response.insert(response.end(),
                          reinterpret_cast<uint8_t*>(&nameLength),
                          reinterpret_cast<uint8_t*>(&nameLength) + sizeof(nameLength));

            // Add server name
            response.insert(response.end(), localPlayerName.begin(), localPlayerName.end());

            // Send multiple times to ensure delivery
            for (int i = 0; i < 3; i++) {
                sendMessage(response.data(), response.size(), senderAddr, senderPort);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            return;
        }
    }

    // Add new peer
    int newPlayerID = peers.size() + 1;  // Server is 0, clients start at 1
    uint32_t playerIDNet = static_cast<uint32_t>(newPlayerID);

    PeerInfo newPeer;
    newPeer.address = senderAddr;
    newPeer.port = senderPort;
    newPeer.isConnected = true;
    newPeer.playerID = newPlayerID;
    newPeer.playerName = playerName;
    newPeer.ping = 0;
    newPeer.lastPingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    peers.push_back(newPeer);
    std::cout << "Added new peer: " << playerName << " (ID: " << newPlayerID << ")" << std::endl;

    // Create connection acceptance message
    std::vector<uint8_t> response;
    response.push_back(MSG_CONNECT_ACCEPT);
    response.insert(response.end(),
                  reinterpret_cast<uint8_t*>(&playerIDNet),
                  reinterpret_cast<uint8_t*>(&playerIDNet) + sizeof(playerIDNet));

    // Add server name length
    uint32_t nameLength = static_cast<uint32_t>(localPlayerName.length());
    response.insert(response.end(),
                  reinterpret_cast<uint8_t*>(&nameLength),
                  reinterpret_cast<uint8_t*>(&nameLength) + sizeof(nameLength));

    // Add server name
    response.insert(response.end(), localPlayerName.begin(), localPlayerName.end());

    // Send multiple times to ensure delivery
    for (int i = 0; i < 3; i++) {
        bool sent = sendMessage(response.data(), response.size(), senderAddr, senderPort);
        std::cout << "Sending connect accept (attempt " << i+1 << "): "
                 << (sent ? "success" : "failed") << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "Player " << playerName << " connected with ID " << newPlayerID << std::endl;
}

// Handle connection acceptance (client side)
void NetworkManager::handleConnectAccept(const uint8_t* data, int size) {
    if (mode != CLIENT || connected) return;

    std::cout << "Processing connection acceptance" << std::endl;

    if (size < 5) {
        std::cerr << "Connection acceptance too short, ignoring" << std::endl;
        return;
    }

    // Extract player ID
    uint32_t assignedID = *reinterpret_cast<const uint32_t*>(&data[1]);
    localPlayerID = assignedID;

    // Extract server name if provided
    std::string serverName = "Server";
    if (size >= 9) {
        uint32_t nameLength = *reinterpret_cast<const uint32_t*>(&data[5]);
        if (size >= 9 + nameLength) {
            serverName = std::string(reinterpret_cast<const char*>(&data[9]), nameLength);
        }
    }

    // Add server as a peer
    PeerInfo serverPeer;
    serverPeer.address = serverAddress;
    serverPeer.port = serverPort;
    serverPeer.isConnected = true;
    serverPeer.playerID = 0; // Server is always ID 0
    serverPeer.playerName = serverName;
    serverPeer.ping = 0;
    serverPeer.lastPingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    peers.push_back(serverPeer);

    // Mark as connected
    connected = true;

    std::cout << "Connected to server '" << serverName << "'. Assigned player ID: " << localPlayerID << std::endl;

    // Send confirmation ping
    uint8_t pingMsg[9];
    pingMsg[0] = MSG_PING;
    *reinterpret_cast<uint32_t*>(&pingMsg[1]) = localPlayerID;
    *reinterpret_cast<uint32_t*>(&pingMsg[5]) = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );

    sendMessage(pingMsg, sizeof(pingMsg), serverAddress, serverPort);
}

// Handle disconnect message
void NetworkManager::handleDisconnect(const uint8_t* data, int size) {
    if (size < 5) {
        return;
    }

    uint32_t playerID = *reinterpret_cast<const uint32_t*>(&data[1]);

    if (mode == CLIENT && playerID == 0) {
        // Server disconnected
        std::cout << "Server disconnected" << std::endl;
        connected = false;
        mode = OFFLINE;
        running.store(false);
        return;
    }

    // Remove peer with this ID
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        if (it->playerID == playerID) {
            std::cout << "Player " << it->playerName << " disconnected" << std::endl;
            peers.erase(it);
            break;
        }
    }

    // If server, forward disconnect to other clients
    if (mode == SERVER) {
        sendToAll(data, size);
    }
}

// Handle ping message
void NetworkManager::handlePing(const std::string& senderAddr, int senderPort, const uint8_t* data, int size) {
    if (size < 9) return;

    uint32_t senderID = *reinterpret_cast<const uint32_t*>(&data[1]);
    uint32_t timestamp = *reinterpret_cast<const uint32_t*>(&data[5]);

    // Update peer's last ping time
    for (auto& peer : peers) {
        if (peer.playerID == senderID) {
            peer.lastPingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
            break;
        }
    }

    // Send pong response
    uint8_t pongMsg[13];
    pongMsg[0] = MSG_PONG;
    *reinterpret_cast<uint32_t*>(&pongMsg[1]) = localPlayerID;

    // Current timestamp
    uint32_t currentTime = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
    *reinterpret_cast<uint32_t*>(&pongMsg[5]) = currentTime;

    // Original timestamp from ping
    *reinterpret_cast<uint32_t*>(&pongMsg[9]) = timestamp;

    sendMessage(pongMsg, sizeof(pongMsg), senderAddr, senderPort);
}

// Handle pong message
void NetworkManager::handlePong(const uint8_t* data, int size) {
    if (size < 13) return;

    uint32_t senderID = *reinterpret_cast<const uint32_t*>(&data[1]);
    uint32_t timestamp = *reinterpret_cast<const uint32_t*>(&data[5]);
    uint32_t originalTimestamp = *reinterpret_cast<const uint32_t*>(&data[9]);

    // Calculate ping time
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    int pingTime = static_cast<int>(currentTime - originalTimestamp);

    // Update peer info
    for (auto& peer : peers) {
        if (peer.playerID == senderID) {
            peer.ping = pingTime;
            peer.lastPingTime = currentTime;
            break;
        }
    }

    // Update average ping
    int totalPing = 0;
    int peerCount = 0;
    for (const auto& peer : peers) {
        totalPing += peer.ping;
        peerCount++;
    }

    if (peerCount > 0) {
        averagePing = totalPing / peerCount;
    }
}

// Handle input update message
void NetworkManager::handleInputUpdate(const uint8_t* data, int size) {
    if (size < sizeof(NetworkInput) + 5) {
        return;
    }

    uint32_t playerID = *reinterpret_cast<const uint32_t*>(&data[1]);

    // Don't process own input
    if (playerID == localPlayerID) {
        return;
    }

    // Extract input data
    NetworkInput input;
    std::memcpy(&input, &data[5], sizeof(NetworkInput));

    // Add to queue
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        inputQueue.push(input);
    }

    // If server, forward to all other clients
    if (mode == SERVER) {
        for (const auto& peer : peers) {
            if (peer.playerID != playerID) {
                sendMessage(data, size, peer.address, peer.port);
            }
        }
    }
}

// Handle game state update from server
void NetworkManager::handleGameStateUpdate(const uint8_t* data, int size) {
    if (mode != CLIENT || size < sizeof(GameStatePacket) + 1) {
        return;
    }

    // Extract game state
    GameStatePacket state;
    std::memcpy(&state, &data[1], sizeof(GameStatePacket));

    // Add to queue
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        // Only queue if it's a newer state
        if (stateQueue.empty() || state.frame > stateQueue.back().frame) {
            stateQueue.push(state);
        }
    }
}

// Send input update to server/clients
void NetworkManager::sendInput(const NetworkInput& input) {
    if (!connected) {
        return;
    }

    std::vector<uint8_t> packet = createInputPacket(input);

    if (mode == CLIENT) {
        sendMessage(packet.data(), packet.size(), serverAddress, serverPort);
    } else if (mode == SERVER) {
        // For server, broadcast to all clients
        sendToAll(packet.data(), packet.size());
    }
}

// Send game state update to clients
void NetworkManager::sendGameState(const GameStatePacket& state) {
    if (!connected || mode != SERVER) {
        return;  // Only the server should send game state updates
    }

    std::vector<uint8_t> packet = createGameStatePacket(state);
    sendToAll(packet.data(), packet.size());
    lastFrameSent = state.frame;
}

// Get remote input from queue
bool NetworkManager::getRemoteInput(NetworkInput& input) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (inputQueue.empty()) {
        return false;
    }

    input = inputQueue.front();
    inputQueue.pop();
    return true;
}

// Get remote game state from queue
bool NetworkManager::getRemoteGameState(GameStatePacket& state) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (stateQueue.empty()) {
        return false;
    }

    state = stateQueue.front();
    stateQueue.pop();
    return true;
}