// NetworkManager.cpp
#include "NetworkManager.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <random>
#include <algorithm>

// Socket implementation with platform-specific code
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

    initialized = true;
    return true;
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

// This patch focuses on fixing the connection handling issues
// in NetworkManager.cpp

// Enhanced startServer method with better error handling and debugging
bool NetworkManager::startServer(int port) {
    if (!initialized || socketHandle == INVALID_SOCKET_HANDLE) {
        std::cerr << "Failed to start server: Network manager not initialized or invalid socket" << std::endl;
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

    // Try to bind multiple times (in case the port is in temporary use)
    bool bindSuccess = false;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (bind(socketHandle, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR_CODE) {
            bindSuccess = true;
            break;
        }

        std::cerr << "Bind attempt " << attempt+1 << " failed: " << GET_SOCKET_ERROR() << std::endl;

        // If port is in use, try again after a brief pause
        if (attempt < 2) {
            std::cout << "Waiting to retry bind..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    if (!bindSuccess) {
        std::cerr << "Failed to bind socket after multiple attempts. Try a different port." << std::endl;
        return false;
    }

    // Set up server state
    mode = SERVER;
    connected = true;
    localPlayerID = 0;  // Server is always player 0

    // Start network thread
    running.store(true);
    networkThread = std::thread(&NetworkManager::handleIncomingMessages, this);

    std::cout << "Server started successfully on port " << serverPort << std::endl;
    std::cout << "Server is now listening for connections (local IP: 127.0.0.1)" << std::endl;
    return true;
}

// Improved connection request handler
void NetworkManager::handleConnectRequest(const std::string& senderAddr, int senderPort, const uint8_t* data, int size) {
    if (mode != SERVER) {
        std::cout << "Ignoring connection request because not in server mode" << std::endl;
        return;
    }

    std::cout << "Processing connection request from " << senderAddr << ":" << senderPort << std::endl;

    // Extract player name from connect request
    std::string playerName = "Player";
    if (size > 5) {
        uint32_t nameLength = *reinterpret_cast<const uint32_t*>(&data[1]);
        std::cout << "Player name length in request: " << nameLength << std::endl;

        if (size >= 5 + nameLength && nameLength < 100) { // Sanity check on name length
            playerName = std::string(reinterpret_cast<const char*>(&data[5]), nameLength);
            std::cout << "Player name extracted: '" << playerName << "'" << std::endl;
        } else {
            std::cout << "Could not extract player name (invalid length or size)" << std::endl;
        }
    } else {
        std::cout << "Request too short to contain player name" << std::endl;
    }

    // Check if we already have this peer
    for (const auto& peer : peers) {
        if (peer.address == senderAddr && peer.port == senderPort) {
            std::cout << "Peer already connected, sending accept message again" << std::endl;

            // Resend accept message
            std::vector<uint8_t> response;
            response.push_back(MSG_CONNECT_ACCEPT);

            // Fixed: Make a copy of the player ID instead of using the const reference directly
            uint32_t playerID = peer.playerID;
            response.insert(response.end(),
                          reinterpret_cast<uint8_t*>(&playerID),
                          reinterpret_cast<uint8_t*>(&playerID) + sizeof(playerID));

            // Add server player name length
            uint32_t nameLength = static_cast<uint32_t>(localPlayerName.length());
            response.insert(response.end(),
                          reinterpret_cast<uint8_t*>(&nameLength),
                          reinterpret_cast<uint8_t*>(&nameLength) + sizeof(nameLength));

            // Add server player name
            response.insert(response.end(), localPlayerName.begin(), localPlayerName.end());

            // Send multiple times to ensure delivery
            for (int i = 0; i < 3; i++) {
                sendMessage(response.data(), response.size(), senderAddr, senderPort);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            return;
        }
    }

    // Generate player ID
    int newPlayerID = peers.size() + 1;  // Server is 0, clients start at 1
    uint32_t playerIDNetworkOrder = static_cast<uint32_t>(newPlayerID);

    // Add peer to list
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

    // Send accept message back
    std::vector<uint8_t> response;
    response.push_back(MSG_CONNECT_ACCEPT);

    // Add assigned player ID
    response.insert(response.end(),
                  reinterpret_cast<uint8_t*>(&playerIDNetworkOrder),
                  reinterpret_cast<uint8_t*>(&playerIDNetworkOrder) + sizeof(playerIDNetworkOrder));

    // Add server player name length
    uint32_t nameLength = static_cast<uint32_t>(localPlayerName.length());
    response.insert(response.end(),
                  reinterpret_cast<uint8_t*>(&nameLength),
                  reinterpret_cast<uint8_t*>(&nameLength) + sizeof(nameLength));

    // Add server player name
    response.insert(response.end(), localPlayerName.begin(), localPlayerName.end());

    // Send multiple times to ensure delivery
    for (int i = 0; i < 3; i++) {
        bool sent = sendMessage(response.data(), response.size(), senderAddr, senderPort);
        std::cout << "Sending connect accept message (attempt " << i+1 << "): "
                  << (sent ? "success" : "failed") << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "Player " << playerName << " connected with ID " << newPlayerID << std::endl;
}

// Improved connection accept handler
void NetworkManager::handleConnectAccept(const uint8_t* data, int size) {
    if (mode != CLIENT) {
        std::cout << "Ignoring connect accept because not in client mode" << std::endl;
        return;
    }

    if (connected) {
        std::cout << "Already connected, ignoring duplicate accept message" << std::endl;
        return;
    }

    // Extract assigned player ID
    if (size < 5) {
        std::cerr << "Connect accept message too short" << std::endl;
        return;
    }

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

    std::cout << "Connected to server. Assigned player ID: " << localPlayerID << std::endl;
}

// Improved sendMessage function with better error handling
bool NetworkManager::sendMessage(const void* data, int size, const std::string& address, int port) {
    if (!initialized || socketHandle == INVALID_SOCKET_HANDLE) {
        std::cerr << "sendMessage: Socket not initialized" << std::endl;
        return false;
    }

    // Debug message type
    if (size > 0) {
        uint8_t msgType = *reinterpret_cast<const uint8_t*>(data);
        std::cout << "sendMessage: Sending message type " << (int)msgType
                 << " to " << address << ":" << port << std::endl;
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
        std::cerr << "Failed to send message to " << address << ":" << port
                 << " - Error: " << GET_SOCKET_ERROR() << std::endl;
        return false;
    }

    std::cout << "Successfully sent " << bytesSent << " bytes to "
              << address << ":" << port << std::endl;
    return true;
}

// In NetworkManager.cpp - improved connectToServer function
bool NetworkManager::connectToServer(const std::string& address, int port) {
    if (!initialized) {
        std::cerr << "ERROR: Network manager not initialized" << std::endl;
        return false;
    }

    serverAddress = address;
    serverPort = port;

    // Set up client state
    mode = CLIENT;

    // Add connection info logging
    std::cout << "Attempting to connect to " << address << ":" << port << std::endl;

    // Send connect request to server with retry logic
    std::vector<uint8_t> connectPacket = createConnectRequestPacket();
    bool sentSuccessfully = false;

    // Try to send connection request multiple times
    for (int attempt = 0; attempt < 3; attempt++) {
        if (sendMessage(connectPacket.data(), connectPacket.size(), serverAddress, serverPort)) {
            sentSuccessfully = true;
            std::cout << "Connect request sent successfully (attempt " << attempt+1 << ")" << std::endl;
            break;
        }
        std::cerr << "Failed to send connect request (attempt " << attempt+1 << ")" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!sentSuccessfully) {
        std::cerr << "Failed to send any connect requests" << std::endl;
        mode = OFFLINE;
        return false;
    }

    // Start network thread
    running.store(true);
    networkThread = std::thread(&NetworkManager::handleIncomingMessages, this);

    // Wait for connect response (with timeout)
    auto startTime = std::chrono::steady_clock::now();
    while (!connected) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Log progress
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count() % 1000 < 100) {
            std::cout << "Waiting for server response..." << std::endl;
        }

        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

        if (elapsed > 5000) {  // 5 second timeout
            std::cerr << "Connection to server timed out after " << elapsed << "ms" << std::endl;
            running.store(false);
            if (networkThread.joinable()) {
                networkThread.join();
            }
            mode = OFFLINE;
            return false;
        }
    }

    std::cout << "Connected to server at " << serverAddress << ":" << serverPort << std::endl;
    return true;
}

void NetworkManager::disconnect() {
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
        // In a real implementation, this would register with a matchmaking service
        std::cout << "Created match: " << matchName << std::endl;
    }

    return success;
}

bool NetworkManager::joinMatch(const std::string& matchCode) {
    if (mode != OFFLINE) {
        return false;
    }

    // In a real implementation, this would lookup the match details from a matchmaking server
    // For this implementation, we assume matchCode is the server IP address
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

void NetworkManager::sendGameState(const GameStatePacket& state) {
    if (!connected || mode != SERVER) {
        return;  // Only the server should send game state updates
    }

    std::vector<uint8_t> packet = createGameStatePacket(state);
    sendToAll(packet.data(), packet.size());
    lastFrameSent = state.frame;
}

bool NetworkManager::getRemoteInput(NetworkInput& input) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (inputQueue.empty()) {
        return false;
    }

    input = inputQueue.front();
    inputQueue.pop();
    return true;
}

bool NetworkManager::getRemoteGameState(GameStatePacket& state) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (stateQueue.empty()) {
        return false;
    }

    state = stateQueue.front();
    stateQueue.pop();
    return true;
}

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

void NetworkManager::sendChatMessage(const std::string& message) {
    if (!connected || message.empty()) {
        return;
    }

    // Construct chat message packet
    std::vector<uint8_t> packet;
    packet.push_back(MSG_CHAT);

    // Add player ID (4 bytes)
    uint32_t playerID = localPlayerID;
    packet.insert(packet.end(), reinterpret_cast<uint8_t*>(&playerID),
                 reinterpret_cast<uint8_t*>(&playerID) + sizeof(playerID));

    // Add message length (2 bytes)
    uint16_t msgLength = static_cast<uint16_t>(message.length());
    packet.insert(packet.end(), reinterpret_cast<uint8_t*>(&msgLength),
                 reinterpret_cast<uint8_t*>(&msgLength) + sizeof(msgLength));

    // Add message content
    packet.insert(packet.end(), message.begin(), message.end());

    if (mode == CLIENT) {
        sendMessage(packet.data(), packet.size(), serverAddress, serverPort);
    } else if (mode == SERVER) {
        // Server broadcasts to all clients
        sendToAll(packet.data(), packet.size());
    }
}

bool NetworkManager::receiveChatMessage(std::string& message) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (chatQueue.empty()) {
        return false;
    }

    message = chatQueue.front();
    chatQueue.pop();
    return true;
}

void NetworkManager::handleIncomingMessages() {
    uint8_t buffer[BUFFER_SIZE];
    sockaddr_in senderAddr;
    socklen_t senderAddrLen = sizeof(senderAddr);

    std::cout << "Network thread started, waiting for messages..." << std::endl;

    while (running.load()) {
        // Clear the buffer
        std::memset(buffer, 0, BUFFER_SIZE);

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

        // Debug log any received message
        std::cout << "Received message from " << senderIP << ":" << senderPort
                 << " (" << bytesReceived << " bytes), type: " << (int)buffer[0] << std::endl;

        // Process message based on type
        uint8_t msgType = buffer[0];

        switch (msgType) {
            case MSG_CONNECT_REQUEST:
                std::cout << "Connection request received from " << senderIP << ":" << senderPort << std::endl;
                handleConnectRequest(senderIP, senderPort, buffer, bytesReceived);
                break;

            case MSG_CONNECT_ACCEPT:
                std::cout << "Connection accepted message received" << std::endl;
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

            case MSG_GAME_START:
                // Handle game start message for clients
                if (mode == CLIENT) {
                    std::cout << "NetworkManager: Received MSG_GAME_START message from host!" << std::endl;
                    // Set the flag so NetworkedGameState can detect it
                    gameStartReceived.store(true);

                    // Immediately send an acknowledgement back to the server
                    uint8_t startAckMsg[1];
                    startAckMsg[0] = MSG_GAME_START_ACK;
                    if (sendMessage(startAckMsg, sizeof(startAckMsg), serverAddress, serverPort)) {
                        std::cout << "NetworkManager: Sent start acknowledgement to server" << std::endl;
                    }

                    // Make sure game start is processed even if message is lost later
                    for (int i = 0; i < 3; i++) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(20));
                        gameStartReceived.store(true);
                    }
                }
                break;

            case MSG_GAME_START_ACK:
                // Server receives acknowledgement that client got the start message
                if (mode == SERVER) {
                    std::cout << "Server received game start acknowledgement from client" << std::endl;
                    // No specific action needed here, just logging
                }
                break;

            case MSG_INPUT_UPDATE:
                handleInputUpdate(buffer, bytesReceived);
                break;

            case MSG_GAME_STATE_UPDATE:
                handleGameStateUpdate(buffer, bytesReceived);
                break;

            case MSG_PING:
                handlePing(senderIP, senderPort, buffer, bytesReceived);
                break;

            case MSG_PONG:
                // Process pong (ping response)
                if (bytesReceived >= 13) {
                    uint32_t senderID = *reinterpret_cast<uint32_t*>(&buffer[1]);
                    uint32_t timestamp = *reinterpret_cast<uint32_t*>(&buffer[5]);
                    uint32_t originalTimestamp = *reinterpret_cast<uint32_t*>(&buffer[9]);

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
                break;

            case MSG_CHAT:
                // Process chat message
                if (bytesReceived >= 7) {
                    uint32_t senderID = *reinterpret_cast<uint32_t*>(&buffer[1]);
                    uint16_t msgLength = *reinterpret_cast<uint16_t*>(&buffer[5]);

                    if (bytesReceived >= 7 + msgLength) {
                        std::string message(reinterpret_cast<char*>(&buffer[7]), msgLength);

                        // Find sender name
                        std::string senderName = "Unknown";
                        for (const auto& peer : peers) {
                            if (peer.playerID == senderID) {
                                senderName = peer.playerName;
                                break;
                            }
                        }

                        // Format and queue the chat message
                        std::string formattedMsg = senderName + ": " + message;

                        {
                            std::lock_guard<std::mutex> lock(queueMutex);
                            chatQueue.push(formattedMsg);
                        }

                        // If server, broadcast to all other clients
                        if (mode == SERVER && senderID != localPlayerID) {
                            for (const auto& peer : peers) {
                                if (peer.playerID != senderID) {
                                    sendMessage(buffer, bytesReceived, peer.address, peer.port);
                                }
                            }
                        }
                    }
                }
                break;

            default:
                std::cerr << "Unknown message type: " << static_cast<int>(msgType) << std::endl;
                break;
        }
    }
    std::cout << "Network thread exiting" << std::endl;
}


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

bool NetworkManager::sendToAll(const void* data, int size) {
    if (mode != SERVER) {
        std::cout << "NetworkManager: sendToAll failed - not in server mode" << std::endl;
        return false;
    }

    // Debug the message being sent
    if (size > 0) {
        uint8_t msgType = *reinterpret_cast<const uint8_t*>(data);
        std::cout << "NetworkManager: sendToAll - Sending message type " << (int)msgType 
                  << " to " << peers.size() << " peers" << std::endl;
        
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
                std::cout << "NetworkManager: Game start message sent to " << sentCount << "/"
                          << peers.size() << " peers (attempt " << retry+1 << ")" << std::endl;

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
            std::cout << "NetworkManager: Failed to send to peer " << peer.playerID
                    << " at " << peer.address << ":" << peer.port << std::endl;
        }
    }
    
    std::cout << "NetworkManager: Message sent to " << sentCount << "/" << peers.size() << " peers" << std::endl;
    return success;
}

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

void NetworkManager::handleInputUpdate(const uint8_t* data, int size) {
    if (size < sizeof(NetworkInput) + 5) {
        return;
    }

    uint32_t playerID = *reinterpret_cast<const uint32_t*>(&data[1]);

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

void NetworkManager::handleGameStateUpdate(const uint8_t* data, int size) {
    if (mode != CLIENT || size < sizeof(GameStatePacket) + 1) {
        return;
    }

    // Extract game state
    GameStatePacket state;
    std::memcpy(&state, &data[1], sizeof(GameStatePacket));

    // Add to queue if it's newer than last received
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        // Only queue if it's a newer state
        if (stateQueue.empty() || state.frame > stateQueue.back().frame) {
            stateQueue.push(state);
        }
    }
}

void NetworkManager::handlePing(const std::string& senderAddr, int senderPort, const uint8_t* data, int size) {
    if (size < 9) {
        return;
    }

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
    uint32_t currentTime = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count());
    *reinterpret_cast<uint32_t*>(&pongMsg[5]) = currentTime;

    // Original timestamp from ping
    *reinterpret_cast<uint32_t*>(&pongMsg[9]) = timestamp;

    sendMessage(pongMsg, sizeof(pongMsg), senderAddr, senderPort);
}

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

std::vector<uint8_t> NetworkManager::createGameStatePacket(const GameStatePacket& state) {
    std::vector<uint8_t> packet;
    packet.push_back(MSG_GAME_STATE_UPDATE);

    // Add game state data
    packet.insert(packet.end(),
                  reinterpret_cast<const uint8_t*>(&state),
                  reinterpret_cast<const uint8_t*>(&state) + sizeof(GameStatePacket));

    return packet;
}

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