// NetworkedGameState.cpp
#include "NetworkedGameState.h"
#include <algorithm>
#include <iostream>
#include <functional> // For std::hash
#include <cstring> // For memset
#include "NetworkManager.h" // For MSG_GAME_START

NetworkedGameState::NetworkedGameState()
    : GameState()
    , networkMode(LOCAL_ONLY)
    , networkFrame(0)
    , serverTickAccumulator(0.0f)
    , serverTickRate(60)  // Default 60 ticks per second
    , clientPredictionEnabled(true)
    , lastAuthoritativeFrame(0)
    , interpolationAlpha(0.0f)
    , inputDelayFrames(2)
    , frameAdvantage(0)
    , syncPercentage(100.0f)
    , newChatMessage(false)
    , spectatorMode(false)
    , rollbackEnabled(false)  // Disabled in client-server model
{
    // Initialize with empty inputs for buffer
    NetworkInput emptyInput = {};
    for (int i = 0; i < 10; i++) {
        inputBuffer.push_back(emptyInput);
    }

    // Ensure players are properly initialized in both network modes
    if (players.size() >= 2) {
        players[0]->stocks = GameConfig::DEFAULT_STOCKS;
        players[0]->damagePercent = 0.0f;
        players[1]->stocks = GameConfig::DEFAULT_STOCKS;
        players[1]->damagePercent = 0.0f;
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
    setNetworkMode(SERVER);

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
        setNetworkMode(SERVER);
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
        case SERVER:
            // Server-specific update logic
            updateAsServer();
            break;

        case CLIENT:
            // Client-specific update logic
            updateAsClient();
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
        Color statusColor = (networkMode == SERVER) ? GREEN : BLUE;
        DrawText(
            (networkMode == SERVER) ? "SERVER" : "CLIENT",
            GameConfig::SCREEN_WIDTH - 100, 10, 20, statusColor
        );

        // Show ping
        int ping = getAveragePing();
        Color pingColor = (ping < 50) ? GREEN : (ping < 100) ? YELLOW : RED;
        DrawText(
            TextFormat("Ping: %d ms", ping),
            GameConfig::SCREEN_WIDTH - 150, 35, 16, pingColor
        );

        // Show frame advantage (in debug mode)
        if (debugMode) {
            DrawText(
                TextFormat("Frame Adv: %d", frameAdvantage),
                GameConfig::SCREEN_WIDTH - 150, 55, 16, WHITE
            );

            DrawText(
                TextFormat("Sync: %.1f%%", syncPercentage),
                GameConfig::SCREEN_WIDTH - 150, 75, 16, WHITE
            );
        }

        // Display chat messages when active
        if (!chatHistory.empty()) {
            int chatY = GameConfig::SCREEN_HEIGHT - 150;

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

        // Apply to remote player - FIXED: Use correct player indices
        if (players.size() >= 2) {
            Character* remotePlayer = (networkMode == SERVER) ? players[1] : players[0];
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

    // Apply to local player - FIXED: Use correct player indices
    Character* localPlayer = (networkMode == SERVER) ? players[0] : players[1];
    applyNetworkInput(localPlayer, currentLocalInput);
}

void NetworkedGameState::synchronizeGameState() {
    NetworkManager& netManager = NetworkManager::getInstance();

    if (networkMode == SERVER) {
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

            // Check for game state information from the host
            if (serverState.extraData >= GameState::GAME_START &&
                serverState.extraData <= GameState::RESULTS_SCREEN &&
                currentState != (GameState::State)serverState.extraData) {

                // Host is in a different state, we need to match it
                GameState::State hostState = (GameState::State)serverState.extraData;
                std::cout << "CLIENT: Host is in state " << hostState
                          << " but client is in state " << currentState
                          << ". Syncing states." << std::endl;

                if (hostState == GameState::GAME_START || hostState == GameState::GAME_PLAYING) {
                    // Force hiding the network UI
                    extern bool showNetworkMenu;
                    showNetworkMenu = false;
                    std::cout << "CLIENT: Setting showNetworkMenu to false" << std::endl;
                }

                // Change to match host state
                changeState(hostState);
            }

            // Calculate local state for comparison
            GameStatePacket localState;
            constructGameStatePacket(localState);

            // Compare checksums to detect desync
            if (serverState.checksum != localState.checksum) {
                if (rollbackEnabled) {
                    // Implement rollback netcode
                    // Find the frame difference
                    int frameDiff = networkFrame - serverState.frame;

                    // Only rollback if frame difference is reasonable
                    if (frameDiff > 0 && frameDiff < 10) {
                        // Rollback simulation
                        // 1. Save current state
                        GameStatePacket currentState;
                        constructGameStatePacket(currentState);

                        // 2. Reapply inputs from the frame of the server state
                        // Find the correct inputs in history (if available)
                        std::deque<NetworkInput> localInputsToReapply;
                        std::deque<NetworkInput> remoteInputsToReapply;

                        // Collect inputs from history that are after the server state frame
                        for (const auto& input : localInputHistory) {
                            if (input.frame >= serverState.frame) {
                                localInputsToReapply.push_front(input);
                            }
                        }

                        for (const auto& input : remoteInputHistory) {
                            if (input.frame >= serverState.frame) {
                                remoteInputsToReapply.push_front(input);
                            }
                        }

                        // 3. Apply server state as starting point
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

                        // 4. Resimulate by applying inputs in order
                        for (int frame = 0; frame < frameDiff; frame++) {
                            // Apply inputs for this frame if available
                            NetworkInput localInput = {};
                            NetworkInput remoteInput = {};

                            // Find inputs for this frame
                            for (const auto& input : localInputsToReapply) {
                                if (input.frame == serverState.frame + frame) {
                                    localInput = input;
                                    break;
                                }
                            }

                            for (const auto& input : remoteInputsToReapply) {
                                if (input.frame == serverState.frame + frame) {
                                    remoteInput = input;
                                    break;
                                }
                            }

                            // Apply inputs to characters - FIXED: Use correct player indices
                            Character* localPlayer = (networkMode == CLIENT) ? players[1] : players[0];
                            Character* remotePlayer = (networkMode == CLIENT) ? players[0] : players[1];

                            applyNetworkInput(localPlayer, localInput);
                            applyNetworkInput(remotePlayer, remoteInput);

                            // Update players for one frame
                            for (auto& player : players) {
                                player->update(platforms);
                            }

                            // Check for character collisions for attacks
                            for (auto& attacker : players) {
                                if (attacker->stateManager.isAttacking) {
                                    for (auto& defender : players) {
                                        if (attacker != defender) {
                                            attacker->checkHit(*defender);
                                        }
                                    }
                                }
                            }
                        }

                        // Update sync percentage based on how close we get after rollback
                        GameStatePacket newState;
                        constructGameStatePacket(newState);

                        if (newState.checksum == serverState.checksum) {
                            // Perfect sync after rollback
                            syncPercentage = 100.0f;
                        } else {
                            // Still some desync, penalize sync percentage
                            syncPercentage = std::max(0.0f, syncPercentage - 0.5f);

                            // Check how different positions are
                            for (int i = 0; i < std::min(2, (int)players.size()); i++) {
                                Vector2 serverPos = serverState.players[i].position;
                                Vector2 localPos = players[i]->physics.position;

                                float dist = std::sqrt(
                                    (serverPos.x - localPos.x) * (serverPos.x - localPos.x) +
                                    (serverPos.y - localPos.y) * (serverPos.y - localPos.y)
                                );

                                // If still a significant difference, apply a subtle correction
                                if (dist > 10.0f) {
                                    // Lerp position with a very small correction factor (to avoid jerky movement)
                                    players[i]->physics.position.x = localPos.x + (serverPos.x - localPos.x) * 0.1f;
                                    players[i]->physics.position.y = localPos.y + (serverPos.y - localPos.y) * 0.1f;
                                }
                            }
                        }
                    } else {
                        // Frame difference too large for rollback, just correct positions
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

void NetworkedGameState::changeState(GameState::State newState) {
    // Handle network-specific state changes
    if (newState == GAME_START && networkMode == SERVER) {
        // Send game start message to all clients
        NetworkManager& netManager = NetworkManager::getInstance();
        uint8_t startGameMsg[1];
        startGameMsg[0] = MSG_GAME_START;

        // Send multiple times to ensure delivery
        for (int i = 0; i < 5; i++) {
            netManager.sendToAll(startGameMsg, sizeof(startGameMsg));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << "Host: Sent game start message to all clients" << std::endl;
    }

    // Call the parent class implementation to handle the actual state change
    GameState::changeState(newState);
}

void NetworkedGameState::applyNetworkInput(Character* character, const NetworkInput& input) {
    if (!character) return;

    // Reset movement, otherwise character keeps moving in one direction
    bool movementInput = false;

    // Apply the input actions
    if (input.moveLeft) {
        character->moveLeft();
        movementInput = true;
    }

    if (input.moveRight) {
        character->moveRight();
        movementInput = true;
    }

    // If no movement input, stop horizontal movement
    if (!movementInput && character->stateManager.state != CharacterState::HITSTUN &&
        character->stateManager.state != CharacterState::ATTACKING) {
        character->physics.velocity.x *= 0.8f; // Apply some friction
    }

    if (input.jump) {
        character->jump();
    }

    if (input.fastFall) {
        character->fastFall();
    }

    // Platform drop-through - using conditions directly rather than a separate field
    if (input.down && input.fastFall &&
        (character->stateManager.state == CharacterState::IDLE ||
         character->stateManager.state == CharacterState::RUNNING)) {
        character->dropThroughPlatform();
    }

    if (input.shield) {
        character->shield();
    } else if (character->stateManager.isShielding) {
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
        // Context-sensitive attack
        if (character->stateManager.state == CharacterState::JUMPING ||
            character->stateManager.state == CharacterState::FALLING) {
            if (input.up) {
                character->upAir();
            } else if (input.down) {
                character->downAir();
            } else if (input.moveLeft && !character->stateManager.isFacingRight) {
                character->forwardAir();
            } else if (input.moveLeft && character->stateManager.isFacingRight) {
                character->backAir();
            } else if (input.moveRight && character->stateManager.isFacingRight) {
                character->forwardAir();
            } else if (input.moveRight && !character->stateManager.isFacingRight) {
                character->backAir();
            } else {
                character->neutralAir();
            }
        } else {
            if (input.up) {
                character->upTilt();
            } else if (input.down) {
                character->downTilt();
            } else if (input.moveLeft || input.moveRight) {
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
        } else if (input.moveLeft || input.moveRight) {
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
        } else if (input.moveLeft || input.moveRight) {
            character->forwardSmash(10.0f);
        }
    }

    // Handle grab
    if (input.grab) {
        character->grab();
    }

    // Handle throws when grabbing
    if (character->stateManager.isGrabbing) {
        if (input.attack) {
            character->pummel();
        } else if (input.moveLeft) {
            character->backThrow();
        } else if (input.moveRight) {
            character->forwardThrow();
        } else if (input.up) {
            character->upThrow();
        } else if (input.down) {
            character->downThrow();
        }
    }
}

void NetworkedGameState::captureLocalInput(NetworkInput& input) {
    // Reset the input
    memset(&input, 0, sizeof(NetworkInput));

    // Keyboard state mapping using the same controls as in Game.cpp
    input.moveLeft = IsKeyDown(KEY_A);
    input.moveRight = IsKeyDown(KEY_D);
    input.jump = IsKeyPressed(KEY_W);
    input.attack = IsKeyPressed(KEY_J);
    input.special = IsKeyPressed(KEY_K);
    input.smashAttack = IsKeyDown(KEY_L);
    input.shield = IsKeyDown(KEY_I);
    input.grab = IsKeyPressed(KEY_U);

    // Direction indicators
    input.up = IsKeyDown(KEY_W);
    input.down = IsKeyDown(KEY_S);

    // Fast fall - updated logic
    input.fastFall = IsKeyDown(KEY_S);

    // Dodge inputs
    input.spot_dodge = IsKeyPressed(KEY_S) && IsKeyDown(KEY_I);
    input.forward_dodge = IsKeyDown(KEY_D) && IsKeyPressed(KEY_I);
    input.backward_dodge = IsKeyDown(KEY_A) && IsKeyPressed(KEY_I);
}

void NetworkedGameState::constructGameStatePacket(GameStatePacket& packet) {
    // Initialize the packet
    memset(&packet, 0, sizeof(GameStatePacket));

    // Set frame number
    packet.frame = networkFrame;

    // Add current game state information
    packet.extraData = (uint32_t)currentState;

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

    // Send the packet
    if (mode == CLIENT) {
        sendMessage(packet.data(), packet.size(), serverAddress, serverPort);
    } else if (mode == SERVER) {
        // Server broadcasts to all clients
        sendToAll(packet.data(), packet.size());
    }

    std::cout << "Chat message sent: " << message << std::endl;
}

// Receive a chat message from the queue
bool NetworkManager::receiveChatMessage(std::string& message) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (chatQueue.empty()) {
        return false;
    }

    message = chatQueue.front();
    chatQueue.pop();
    return true;
}

// Send authoritative game state to all clients (server only)
void NetworkedGameState::sendServerStateUpdate() {
    NetworkManager& netManager = NetworkManager::getInstance();

    // Create game state packet
    GameStatePacket statePacket;
    constructGameStatePacket(statePacket);

    // Send to all clients
    netManager.sendGameState(statePacket);
    
    // Debug output
    std::cout << "Server sending state update for frame " << networkFrame << std::endl;
}

// Server-specific update logic
void NetworkedGameState::updateAsServer() {
    // Only process game logic if in GAME_PLAYING state
    if (currentState == GAME_PLAYING) {
        // Process remote inputs from clients
        NetworkManager& netManager = NetworkManager::getInstance();
        NetworkInput clientInput;

        // Process all pending client inputs
        while (netManager.getRemoteInput(clientInput)) {
            // For simplicity, the client id is always 1 (player 1)
            // Server is always player 0
            int clientId = 1;
            
            // Apply input to the appropriate player character
            if (clientId > 0 && clientId < players.size()) {
                Character* character = players[clientId];
                applyNetworkInput(character, clientInput);
                
                // Debug output
                std::cout << "Server received input from client with ID " << clientId 
                          << " for frame " << clientInput.frame << std::endl;
            }
        }

        // Calculate time between ticks for fixed update rate
        float deltaTime = GetFrameTime();
        serverTickAccumulator += deltaTime;

        // Fixed update at server tick rate
        float tickInterval = 1.0f / serverTickRate;
        while (serverTickAccumulator >= tickInterval) {
            // Process server's own input
            sendLocalInput();
            
            // Update game state
            GameState::update();

            // Update network frame counter
            networkFrame++;

            // Send authoritative state to clients periodically
            if (networkFrame % 2 == 0) { // Every 2 frames (~30Hz with 60fps tick rate)
                sendServerStateUpdate();
            }

            serverTickAccumulator -= tickInterval;
        }
        
        // Always update players for collisions/movement even outside fixed updates
        for (auto& player : players) {
            player->update(platforms);
        }
        
        // Check for character collisions for attacks
        for (auto& attacker : players) {
            if (attacker->stateManager.isAttacking) {
                for (auto& defender : players) {
                    if (attacker != defender) {
                        attacker->checkHit(*defender);
                    }
                }
            }
        }
    } else {
        // For menus, etc. just use normal update
        GameState::update();

        // Special handling for game start
        if (currentState == GAME_START) {
            // Notify clients when game is starting
            if (stateTimer == 0) {
                // Send game start message to all clients
                NetworkManager& netManager = NetworkManager::getInstance();
                uint8_t startGameMsg[1];
                startGameMsg[0] = MSG_GAME_START;

                // Send multiple times to ensure delivery
                for (int i = 0; i < 5; i++) {
                    netManager.sendToAll(startGameMsg, sizeof(startGameMsg));
                }
                std::cout << "Server: Sent game start message to all clients" << std::endl;
            }
        }
    }
}

// Client-specific update logic
void NetworkedGameState::updateAsClient() {
    // Check if server has started the game
    NetworkManager& netManager = NetworkManager::getInstance();
    bool gameStarted = netManager.hasGameStartMessage();

    if (gameStarted && currentState != GAME_PLAYING && currentState != GAME_START) {
        std::cout << "Client: Received game start message - starting game!" << std::endl;

        // Set the network UI state to HIDDEN in Game.cpp
        extern bool showNetworkMenu;
        showNetworkMenu = false;

        // Change to game start state
        changeState(GameState::GAME_START);
        
        // Wait a moment to process
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Force player updates even in non-gameplay states for smooth transition
    for (auto& player : players) {
        player->update(platforms);
    }

    if (currentState == GAME_PLAYING) {
        // In spectator mode, we only receive and display state
        if (spectatorMode) {
            // Apply server state
            GameStatePacket serverState;
            if (netManager.getRemoteGameState(serverState)) {
                // Directly apply the server state
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
            
            // Just increment the frame counter
            networkFrame++;
        } 
        else {
            // Normal client mode - capture and send input to server
            sendLocalInput();

            // Always run simulation for responsiveness but with client prediction
            if (clientPredictionEnabled) {
                // Update character physics
                int clientId = netManager.getPlayerID();
                if (clientId > 0 && clientId < players.size()) {
                    // Only apply physics for client's own character
                    players[clientId]->update(platforms);
                }
            }

            // Apply server corrections
            GameStatePacket serverState;
            if (netManager.getRemoteGameState(serverState)) {
                std::cout << "Client received state update from server" << std::endl;
                applyServerState(serverState);
            }

            // Update network frame
            networkFrame++;
        }
    } else {
        // For menus, etc. just use normal update
        GameState::update();
    }
}

// Apply server state updates to client (client only)
void NetworkedGameState::applyServerState(const GameStatePacket& serverState) {
    // If this is a very old state, ignore it
    if (serverState.frame < lastAuthoritativeFrame) {
        return;
    }

    // Update last authoritative frame
    lastAuthoritativeFrame = serverState.frame;
    
    // Debug output
    std::cout << "Client applying server state from frame " << serverState.frame 
              << " at client frame " << networkFrame << std::endl;

    // Check for game state information from server
    if (serverState.extraData >= GameState::GAME_START &&
        serverState.extraData <= GameState::RESULTS_SCREEN &&
        currentState != (GameState::State)serverState.extraData) {

        // Server is in a different state, we need to match it
        GameState::State serverGameState = (GameState::State)serverState.extraData;
        std::cout << "CLIENT: Server is in state " << serverGameState
                  << " but client is in state " << currentState
                  << ". Syncing states." << std::endl;

        if (serverGameState == GameState::GAME_START || serverGameState == GameState::GAME_PLAYING) {
            // Force hiding the network UI
            extern bool showNetworkMenu;
            showNetworkMenu = false;
            std::cout << "CLIENT: Setting showNetworkMenu to false" << std::endl;
        }

        // Change to match server's state
        changeState(serverGameState);
    }

    // Calculate the frame difference
    int frameDiff = networkFrame - serverState.frame;

    // Client-side reconciliation based on server authority
    int clientId = NetworkManager::getInstance().getPlayerID();
    if (clientId >= 0 && clientId < players.size()) {
        // Update player 0 (server-controlled) directly from server state
        if (players.size() > 0) {
            players[0]->physics.position = serverState.players[0].position;
            players[0]->physics.velocity = serverState.players[0].velocity;
            players[0]->damagePercent = serverState.players[0].damagePercent;
            players[0]->stocks = serverState.players[0].stocks;
            players[0]->stateManager.state = (CharacterState)(serverState.players[0].stateID);
            players[0]->stateManager.isFacingRight = serverState.players[0].isFacingRight;
            players[0]->stateManager.isAttacking = serverState.players[0].isAttacking;
            players[0]->stateManager.currentAttack = (AttackType)(serverState.players[0].currentAttack);
            players[0]->stateManager.attackFrame = serverState.players[0].attackFrame;
        }
        
        // For client's own character, apply smoothed correction only if significant error
        if (clientId < 2) { // Players array has fixed size of 2
            Vector2 serverPos = serverState.players[clientId].position;
            Vector2 clientPos = players[clientId]->physics.position;
            
            float dist = std::sqrt(
                (serverPos.x - clientPos.x) * (serverPos.x - clientPos.x) +
                (serverPos.y - clientPos.y) * (serverPos.y - clientPos.y)
            );
            
            // Apply correction based on distance
            if (dist > 30.0f) {
                // Major desync - stronger correction
                players[clientId]->physics.position.x = clientPos.x + (serverPos.x - clientPos.x) * 0.5f;
                players[clientId]->physics.position.y = clientPos.y + (serverPos.y - clientPos.y) * 0.5f;
                // Also adopt server velocity for major corrections
                players[clientId]->physics.velocity = serverState.players[clientId].velocity;
                syncPercentage = 60.0f;
            }
            else if (dist > 10.0f) {
                // Minor desync - gentle correction
                players[clientId]->physics.position.x = clientPos.x + (serverPos.x - clientPos.x) * 0.2f;
                players[clientId]->physics.position.y = clientPos.y + (serverPos.y - clientPos.y) * 0.2f;
                syncPercentage = 80.0f;
            }
            else {
                // Good sync - minimal correction
                syncPercentage = 100.0f;
            }
        }
        
        // Critical state always comes from server (stocks, damage, etc.)
        if (clientId < 2) { // Players array has fixed size of 2
            players[clientId]->stocks = serverState.players[clientId].stocks;
            players[clientId]->damagePercent = serverState.players[clientId].damagePercent;
            
            // For attack state, server is authoritative
            bool serverIsAttacking = serverState.players[clientId].isAttacking;
            if (serverIsAttacking != players[clientId]->stateManager.isAttacking) {
                players[clientId]->stateManager.isAttacking = serverIsAttacking;
                players[clientId]->stateManager.currentAttack = (AttackType)(serverState.players[clientId].currentAttack);
                players[clientId]->stateManager.attackFrame = serverState.players[clientId].attackFrame;
            }
        }
    }
}
