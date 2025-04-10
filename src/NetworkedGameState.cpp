// NetworkedGameState.cpp
#include "NetworkedGameState.h"
#include <algorithm>
#include <iostream>
#include <functional> // For std::hash
#include <cstring> // For memset

NetworkedGameState::NetworkedGameState()
    : GameState()
    , networkMode(LOCAL_ONLY)
    , networkFrame(0)
    , inputDelayFrames(2)
    , frameAdvantage(0)
    , syncPercentage(100.0f)
    , newChatMessage(false)
    , spectatorMode(false)
    , rollbackEnabled(true)
{
    // Initialize with empty inputs for buffer
    NetworkInput emptyInput = {};
    for (int i = 0; i < 10; i++) {
        inputBuffer.push_back(emptyInput);
    }
}

void NetworkedGameState::setNetworkMode(NetworkGameMode mode) {
    if (networkMode == mode) {
        return;
    }

    // Clean up old mode if needed
    if (networkMode != LOCAL_ONLY) {
        disconnectFromGame();
    }

    networkMode = mode;

    // Initialize new mode
    if (networkMode != LOCAL_ONLY) {
        // Reset network state
        networkFrame = 0;
        stateBuffer.clear();
        inputBuffer.clear();
        localInputHistory.clear();
        remoteInputHistory.clear();

        // Initialize with empty inputs for buffer
        NetworkInput emptyInput = {};
        for (int i = 0; i < 10; i++) {
            inputBuffer.push_back(emptyInput);
        }
    }
}

bool NetworkedGameState::hostGame(int port) {
    // Initialize network manager if not already
    NetworkManager& netManager = NetworkManager::getInstance();
    if (!netManager.initialize()) {
        std::cerr << "Failed to initialize network manager" << std::endl;
        return false;
    }

    // Start server
    if (!netManager.startServer(port)) {
        std::cerr << "Failed to start server" << std::endl;
        return false;
    }

    // Set network mode
    setNetworkMode(HOST);

    // Reset game state
    resetMatch();

    return true;
}

bool NetworkedGameState::joinGame(const std::string& address, int port) {
    // Initialize network manager if not already
    NetworkManager& netManager = NetworkManager::getInstance();
    if (!netManager.initialize()) {
        std::cerr << "Failed to initialize network manager" << std::endl;
        return false;
    }

    // Connect to server
    if (!netManager.connectToServer(address, port)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return false;
    }

    // Set network mode
    setNetworkMode(CLIENT);

    // Reset game state
    resetMatch();

    return true;
}

void NetworkedGameState::disconnectFromGame() {
    // Disconnect from network
    NetworkManager& netManager = NetworkManager::getInstance();
    if (netManager.isConnected()) {
        netManager.disconnect();
    }

    // Reset to local mode
    networkMode = LOCAL_ONLY;
}

bool NetworkedGameState::createNetworkMatch(const std::string& matchName) {
    NetworkManager& netManager = NetworkManager::getInstance();
    if (netManager.createMatch(matchName)) {
        setNetworkMode(HOST);
        return true;
    }
    return false;
}

bool NetworkedGameState::joinNetworkMatch(const std::string& matchCode) {
    NetworkManager& netManager = NetworkManager::getInstance();
    if (netManager.joinMatch(matchCode)) {
        setNetworkMode(CLIENT);
        return true;
    }
    return false;
}

std::vector<std::string> NetworkedGameState::getAvailableMatches() {
    NetworkManager& netManager = NetworkManager::getInstance();
    return netManager.getAvailableMatches();
}

void NetworkedGameState::update() {
    // Update the network manager
    NetworkManager& netManager = NetworkManager::getInstance();
    netManager.update();

    // If not connected and in network mode, return to title screen
    if (!netManager.isConnected() && networkMode != LOCAL_ONLY) {
        setNetworkMode(LOCAL_ONLY);
        changeState(TITLE_SCREEN);
        return;
    }

    // Process network messages
    processRemoteInput();

    // Handle chat messages
    std::string chatMsg;
    while (netManager.receiveChatMessage(chatMsg)) {
        chatHistory.push_back(chatMsg);
        if (chatHistory.size() > 20) {
            chatHistory.erase(chatHistory.begin());
        }
        newChatMessage = true;
    }

    // Execute game state based on network mode
    switch (networkMode) {
        case HOST:
            // As host, we are the authority on game state
            if (currentState == GAME_PLAYING) {
                // Process local input first
                sendLocalInput();

                // Update game
                GameState::update();

                // Synchronize game state to clients
                synchronizeGameState();

                // Increment network frame
                networkFrame++;
            } else {
                // For menus, etc. just use normal update
                GameState::update();
            }
            break;

        case CLIENT:
            // As client, we follow the host's state
            if (currentState == GAME_PLAYING) {
                // Process input
                sendLocalInput();

                // Update game
                GameState::update();

                // Apply any state corrections from server
                synchronizeGameState();

                // Increment network frame
                networkFrame++;
            } else {
                // For menus, etc. just use normal update
                GameState::update();
            }
            break;

        case LOCAL_ONLY:
        default:
            // Use normal update for local play
            GameState::update();
            break;
    }
}

void NetworkedGameState::draw() {
    // Draw the game as normal
    GameState::draw();

    // Add network-specific UI elements
    if (networkMode != LOCAL_ONLY) {
        // Show connection status
        Color statusColor = (networkMode == HOST) ? GREEN : BLUE;
        DrawText(
            (networkMode == HOST) ? "SERVER" : "CLIENT",
            SCREEN_WIDTH - 100, 10, 20, statusColor
        );

        // Show ping
        int ping = getAveragePing();
        Color pingColor = (ping < 50) ? GREEN : (ping < 100) ? YELLOW : RED;
        DrawText(
            TextFormat("Ping: %d ms", ping),
            SCREEN_WIDTH - 150, 35, 16, pingColor
        );

        // Show frame advantage (in debug mode)
        if (debugMode) {
            DrawText(
                TextFormat("Frame Adv: %d", frameAdvantage),
                SCREEN_WIDTH - 150, 55, 16, WHITE
            );

            DrawText(
                TextFormat("Sync: %.1f%%", syncPercentage),
                SCREEN_WIDTH - 150, 75, 16, WHITE
            );
        }

        // Display chat messages when active
        if (!chatHistory.empty()) {
            int chatY = SCREEN_HEIGHT - 150;

            // Draw chat background
            DrawRectangle(10, chatY - 5, 400, 125, Fade(BLACK, 0.7f));

            // Display most recent messages (up to 5)
            int msgCount = std::min(5, (int)chatHistory.size());
            for (int i = 0; i < msgCount; i++) {
                int idx = chatHistory.size() - msgCount + i;
                DrawText(
                    chatHistory[idx].c_str(),
                    20, chatY + i * 20, 16,
                    (i == msgCount - 1 && newChatMessage) ? YELLOW : WHITE
                );
            }

            // Reset new message flag after a few seconds
            static int chatDisplayTimer = 0;
            if (newChatMessage) {
                chatDisplayTimer++;
                if (chatDisplayTimer > 180) {  // 3 seconds at 60 FPS
                    newChatMessage = false;
                    chatDisplayTimer = 0;
                }
            }
        }
    }
}

void NetworkedGameState::processRemoteInput() {
    if (networkMode == LOCAL_ONLY) {
        return;
    }

    NetworkManager& netManager = NetworkManager::getInstance();
    NetworkInput remoteInput;

    // Process any pending remote inputs
    while (netManager.getRemoteInput(remoteInput)) {
        remoteInputQueue.push(remoteInput);
    }

    // Apply remote input if available
    if (!remoteInputQueue.empty()) {
        currentRemoteInput = remoteInputQueue.front();
        remoteInputQueue.pop();

        // Store in history
        remoteInputHistory.push_front(currentRemoteInput);
        if (remoteInputHistory.size() > 60) {
            remoteInputHistory.pop_back();
        }

        // Check frame advantage (how far ahead/behind we are)
        if (currentRemoteInput.frame > 0) {
            frameAdvantage = networkFrame - currentRemoteInput.frame;
        }

        // Apply to remote player
        if (players.size() >= 2) {
            Character* remotePlayer = (networkMode == HOST) ? players[1] : players[0];
            applyNetworkInput(remotePlayer, currentRemoteInput);
        }
    }
}

void NetworkedGameState::sendLocalInput() {
    if (networkMode == LOCAL_ONLY) {
        return;
    }

    // Capture current input
    captureLocalInput(currentLocalInput);
    currentLocalInput.frame = networkFrame;

    // Store in history
    localInputHistory.push_front(currentLocalInput);
    if (localInputHistory.size() > 60) {
        localInputHistory.pop_back();
    }

    // Send to remote
    NetworkManager& netManager = NetworkManager::getInstance();
    netManager.sendInput(currentLocalInput);

    // Apply to local player
    Character* localPlayer = (networkMode == HOST) ? players[0] : players[1];
    applyNetworkInput(localPlayer, currentLocalInput);
}

void NetworkedGameState::synchronizeGameState() {
    NetworkManager& netManager = NetworkManager::getInstance();

    if (networkMode == HOST) {
        // Server: send authoritative game state periodically
        if (networkFrame % 10 == 0) {  // Every 10 frames
            GameStatePacket statePacket;
            constructGameStatePacket(statePacket);
            netManager.sendGameState(statePacket);
        }
    } else if (networkMode == CLIENT) {
        // Client: process state updates from server
        GameStatePacket serverState;
        if (netManager.getRemoteGameState(serverState)) {
            // Store in buffer
            stateBuffer.push_front(serverState);
            if (stateBuffer.size() > 10) {
                stateBuffer.pop_back();
            }

            // Calculate local state for comparison
            GameStatePacket localState;
            constructGameStatePacket(localState);

            // Compare checksums to detect desync
            if (serverState.checksum != localState.checksum) {
                if (rollbackEnabled) {
                    // TODO: Implement rollback netcode
                    // For now, just correct positions
                    for (int i = 0; i < std::min(2, (int)players.size()); i++) {
                        // Smooth correction towards server state
                        Vector2 serverPos = serverState.players[i].position;
                        Vector2 localPos = players[i]->physics.position;

                        float dist = std::sqrt(
                            (serverPos.x - localPos.x) * (serverPos.x - localPos.x) +
                            (serverPos.y - localPos.y) * (serverPos.y - localPos.y)
                        );

                        // Only correct if significant difference
                        if (dist > 5.0f) {
                            // Lerp position
                            players[i]->physics.position.x = localPos.x + (serverPos.x - localPos.x) * 0.3f;
                            players[i]->physics.position.y = localPos.y + (serverPos.y - localPos.y) * 0.3f;

                            // Adopt server velocity
                            players[i]->physics.velocity = serverState.players[i].velocity;

                            // Update sync percentage
                            syncPercentage = std::max(0.0f, syncPercentage - 1.0f);
                        } else {
                            // Gradually restore sync percentage
                            syncPercentage = std::min(100.0f, syncPercentage + 0.1f);
                        }
                    }
                } else {
                    // Without rollback, just adopt server state
                    for (int i = 0; i < std::min(2, (int)players.size()); i++) {
                        players[i]->physics.position = serverState.players[i].position;
                        players[i]->physics.velocity = serverState.players[i].velocity;
                        players[i]->damagePercent = serverState.players[i].damagePercent;
                        players[i]->stocks = serverState.players[i].stocks;
                        players[i]->stateManager.state = (CharacterState)(serverState.players[i].stateID);
                        players[i]->stateManager.isFacingRight = serverState.players[i].isFacingRight;
                        players[i]->stateManager.isAttacking = serverState.players[i].isAttacking;
                        players[i]->stateManager.currentAttack = (AttackType)(serverState.players[i].currentAttack);
                        players[i]->stateManager.attackFrame = serverState.players[i].attackFrame;
                    }
                }
            } else {
                // States match, gradually restore sync percentage
                syncPercentage = std::min(100.0f, syncPercentage + 0.5f);
            }
        }
    }
}

int NetworkedGameState::getAveragePing() const {
    NetworkManager& netManager = NetworkManager::getInstance();
    return netManager.getAveragePing();
}

void NetworkedGameState::sendChatMessage(const std::string& message) {
    if (networkMode == LOCAL_ONLY || message.empty()) {
        return;
    }

    NetworkManager& netManager = NetworkManager::getInstance();
    netManager.sendChatMessage(message);

    // Add to local chat history
    std::string playerName = netManager.getLocalPlayerName();
    std::string formattedMsg = playerName + ": " + message;
    chatHistory.push_back(formattedMsg);

    if (chatHistory.size() > 20) {
        chatHistory.erase(chatHistory.begin());
    }

    newChatMessage = true;
}

bool NetworkedGameState::receiveChatMessage(std::string& message) {
    // This method is mainly for external UI interaction
    if (chatHistory.empty() || !newChatMessage) {
        return false;
    }

    message = chatHistory.back();
    newChatMessage = false;
    return true;
}

void NetworkedGameState::applyNetworkInput(Character* character, const NetworkInput& input) {
    if (!character) return;

    // Reset states first
    bool wasMovingLeft = false;
    bool wasMovingRight = false;
    bool wasJumping = false;
    bool wasAttacking = false;
    bool wasShielding = false;
    bool wasFastFalling = false;

    // Apply the input actions
    if (input.moveLeft) {
        wasMovingLeft = true;
        character->moveLeft();
    }

    if (input.moveRight) {
        wasMovingRight = true;
        character->moveRight();
    }

    if (input.jump) {
        wasJumping = true;
        character->jump();
    }

    if (input.fastFall) {
        wasFastFalling = true;
        character->fastFall();
    }

    if (input.shield) {
        wasShielding = true;
        character->shield();
    } else if (wasShielding) {
        character->releaseShield();
    }

    // Handle dodges
    if (input.spot_dodge) {
        character->spotDodge();
    } else if (input.forward_dodge) {
        character->forwardDodge();
    } else if (input.backward_dodge) {
        character->backDodge();
    }

    // Handle attacks
    if (input.attack) {
        wasAttacking = true;
        // Context-sensitive attack
        if (character->stateManager.state == JUMPING || character->stateManager.state == FALLING) {
            if (input.up) {
                character->upAir();
            } else if (input.down) {
                character->downAir();
            } else if (wasMovingLeft) {
                character->backAir();
            } else if (wasMovingRight) {
                character->forwardAir();
            } else {
                character->neutralAir();
            }
        } else {
            if (input.up) {
                character->upTilt();
            } else if (input.down) {
                character->downTilt();
            } else if (wasMovingLeft || wasMovingRight) {
                character->forwardTilt();
            } else {
                character->jab();
            }
        }
    }

    // Handle special attacks
    if (input.special) {
        if (input.up) {
            character->upSpecial();
        } else if (input.down) {
            character->downSpecial();
        } else if (wasMovingLeft || wasMovingRight) {
            character->sideSpecial();
        } else {
            character->neutralSpecial();
        }
    }

    // Handle smash attacks
    if (input.smashAttack) {
        if (input.up) {
            character->upSmash(10.0f); // Default charge
        } else if (input.down) {
            character->downSmash(10.0f);
        } else if (wasMovingLeft || wasMovingRight) {
            character->forwardSmash(10.0f);
        }
    }

    // Handle grab
    if (input.grab) {
        character->grab();
    }
}

void NetworkedGameState::captureLocalInput(NetworkInput& input) {
    // Reset the input
    memset(&input, 0, sizeof(NetworkInput));

    // Check keyboard state
    input.moveLeft = IsKeyDown(KEY_A);
    input.moveRight = IsKeyDown(KEY_D);
    input.jump = IsKeyPressed(KEY_W);
    input.fastFall = IsKeyDown(KEY_S);
    input.attack = IsKeyPressed(KEY_J);
    input.special = IsKeyPressed(KEY_K);
    input.smashAttack = IsKeyDown(KEY_L);
    input.shield = IsKeyDown(KEY_I);
    input.grab = IsKeyPressed(KEY_U);
    input.spot_dodge = IsKeyPressed(KEY_S) && IsKeyDown(KEY_I);
    input.forward_dodge = IsKeyPressed(KEY_A) && IsKeyDown(KEY_I);
    input.backward_dodge = IsKeyPressed(KEY_D) && IsKeyDown(KEY_I);
    input.up = IsKeyDown(KEY_W);
    input.down = IsKeyDown(KEY_S);
}

void NetworkedGameState::constructGameStatePacket(GameStatePacket& packet) {
    // Initialize the packet
    memset(&packet, 0, sizeof(GameStatePacket));

    // Set frame number
    packet.frame = networkFrame;

    // Fill player states
    for (int i = 0; i < std::min(2, (int)players.size()); i++) {
        Character* player = players[i];

        if (!player) continue;

        packet.players[i].position = player->physics.position;
        packet.players[i].velocity = player->physics.velocity;
        packet.players[i].damagePercent = player->damagePercent;
        packet.players[i].stocks = player->stocks;
        packet.players[i].stateID = static_cast<int>(player->stateManager.state);
        packet.players[i].isFacingRight = player->stateManager.isFacingRight;
        packet.players[i].isAttacking = player->stateManager.isAttacking;
        packet.players[i].currentAttack = static_cast<int>(player->stateManager.currentAttack);
        packet.players[i].attackFrame = player->stateManager.attackFrame;
    }

    // Calculate checksum for the packet
    packet.checksum = calculateStateChecksum(packet);
}

uint32_t NetworkedGameState::calculateStateChecksum(const GameStatePacket& state) {
    // Simple checksum calculation
    std::hash<float> float_hash;
    std::hash<int> int_hash;
    std::hash<bool> bool_hash;

    size_t hash_val = 0;

    // Combine values from both players
    for (int i = 0; i < 2; i++) {
        hash_val ^= float_hash(state.players[i].position.x) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        hash_val ^= float_hash(state.players[i].position.y) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        hash_val ^= float_hash(state.players[i].velocity.x) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        hash_val ^= float_hash(state.players[i].velocity.y) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        hash_val ^= float_hash(state.players[i].damagePercent) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        hash_val ^= int_hash(state.players[i].stocks) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        hash_val ^= int_hash(state.players[i].stateID) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        hash_val ^= bool_hash(state.players[i].isFacingRight) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        hash_val ^= bool_hash(state.players[i].isAttacking) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        hash_val ^= int_hash(state.players[i].currentAttack) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
        hash_val ^= int_hash(state.players[i].attackFrame) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
    }

    // Add frame to the hash
    hash_val ^= int_hash(state.frame) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);

    // Convert to 32-bit uint
    return static_cast<uint32_t>(hash_val & 0xFFFFFFFF);
}