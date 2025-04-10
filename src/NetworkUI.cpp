// NetworkUI.cpp
#include "NetworkUI.h"
#include <cmath> // For fabs
#include <iostream>

// Button implementation
void Button::draw() {
    if (!isVisible) return;

    Color bgColor = DARKGRAY;
    Color textColor = WHITE;

    if (isHovered) {
        bgColor = GRAY;
    }

    if (isActive) {
        bgColor = LIGHTGRAY;
        textColor = BLACK;
    }

    DrawRectangleRec(bounds, bgColor);
    DrawRectangleLinesEx(bounds, 2, BLACK);

    // Center text in button
    int textWidth = MeasureText(text.c_str(), 20);
    int textX = bounds.x + (bounds.width - textWidth) / 2;
    int textY = bounds.y + (bounds.height - 20) / 2;

    DrawText(text.c_str(), textX, textY, 20, textColor);
}

bool Button::update() {
    if (!isVisible) return false;

    Vector2 mousePos = GetMousePosition();
    bool wasActive = isActive;
    bool wasHovered = isHovered;

    isHovered = CheckCollisionPointRec(mousePos, bounds);

    if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isActive = true;

        // Execute callback
        if (onClick) {
            onClick();
        }
    } else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        isActive = false;
    }

    return wasActive != isActive || wasHovered != isHovered;
}

// Label implementation
void Label::draw() {
    if (!isVisible) return;

    DrawText(text.c_str(), bounds.x, bounds.y, fontSize, textColor);
}

bool Label::update() {
    // No interactive behavior for labels
    return false;
}

// TextInput implementation
void TextInput::draw() {
    if (!isVisible) return;

    // Draw the text input field
    Color bgColor = isEditing ? LIGHTGRAY : DARKGRAY;
    DrawRectangleRec(bounds, bgColor);
    DrawRectangleLinesEx(bounds, isEditing ? 2 : 1, isEditing ? BLUE : BLACK);

    // Draw text or placeholder
    const char* displayText = text.empty() ? placeholder.c_str() : text.c_str();
    Color textColor = text.empty() ? GRAY : BLACK;

    DrawText(displayText, bounds.x + 5, bounds.y + (bounds.height - 20) / 2, 20, textColor);

    // Draw cursor if editing
    if (isEditing) {
        // Calculate cursor position
        int textWidth = MeasureText(text.substr(0, cursorPos).c_str(), 20);
        int cursorX = bounds.x + 5 + textWidth;
        int cursorY = bounds.y + 5;

        DrawRectangle(cursorX, cursorY, 2, bounds.height - 10, BLACK);
    }
}

bool TextInput::update() {
    if (!isVisible) return false;

    Vector2 mousePos = GetMousePosition();
    bool wasEditing = isEditing;

    // Check if clicked
    if (CheckCollisionPointRec(mousePos, bounds)) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isEditing = true;

            // Set cursor position based on click (approximate)
            int clickX = mousePos.x - bounds.x - 5;
            int bestPos = 0;
            int minDiff = 1000;

            for (int i = 0; i <= text.length(); i++) {
                int width = MeasureText(text.substr(0, i).c_str(), 20);
                int diff = abs(width - clickX);

                if (diff < minDiff) {
                    minDiff = diff;
                    bestPos = i;
                }
            }

            cursorPos = bestPos;
        }
    } else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isEditing = false;
    }

    // Process key input when editing
    if (isEditing) {
        int key = GetCharPressed();

        // Add typed characters
        while (key > 0) {
            // Only allow visible ASCII characters
            if (key >= 32 && key <= 125) {
                text.insert(cursorPos, 1, static_cast<char>(key));
                cursorPos++;
            }

            key = GetCharPressed();
        }

        // Handle backspace
        if (IsKeyPressed(KEY_BACKSPACE) && cursorPos > 0) {
            text.erase(cursorPos - 1, 1);
            cursorPos--;
        }

        // Handle delete
        if (IsKeyPressed(KEY_DELETE) && cursorPos < text.length()) {
            text.erase(cursorPos, 1);
        }

        // Handle arrow keys
        if (IsKeyPressed(KEY_LEFT) && cursorPos > 0) {
            cursorPos--;
        }

        if (IsKeyPressed(KEY_RIGHT) && cursorPos < text.length()) {
            cursorPos++;
        }

        // Handle home and end
        if (IsKeyPressed(KEY_HOME)) {
            cursorPos = 0;
        }

        if (IsKeyPressed(KEY_END)) {
            cursorPos = text.length();
        }

        // Submit with enter
        if (IsKeyPressed(KEY_ENTER)) {
            isEditing = false;
        }
    }

    return wasEditing != isEditing;
}

// Checkbox implementation
void Checkbox::draw() {
    if (!isVisible) return;

    // Draw checkbox
    Rectangle checkboxRect = {bounds.x, bounds.y, 20, 20};
    DrawRectangleRec(checkboxRect, WHITE);
    DrawRectangleLinesEx(checkboxRect, 1, BLACK);

    // Draw checkmark if checked
    if (isChecked) {
        DrawLine(checkboxRect.x + 4, checkboxRect.y + 10, checkboxRect.x + 8, checkboxRect.y + 16, BLACK);
        DrawLine(checkboxRect.x + 8, checkboxRect.y + 16, checkboxRect.x + 16, checkboxRect.y + 4, BLACK);
    }

    // Draw label
    DrawText(text.c_str(), bounds.x + 30, bounds.y, 20, WHITE);
}

bool Checkbox::update() {
    if (!isVisible) return false;

    Vector2 mousePos = GetMousePosition();
    bool wasChecked = isChecked;

    // Create clickable area
    Rectangle clickArea = {bounds.x, bounds.y, bounds.width, 20};

    if (CheckCollisionPointRec(mousePos, clickArea)) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isChecked = !isChecked;
        }
    }

    return wasChecked != isChecked;
}

// Dropdown implementation
void Dropdown::draw() {
    if (!isVisible) return;

    // Draw main dropdown box
    DrawRectangleRec(bounds, DARKGRAY);
    DrawRectangleLinesEx(bounds, 1, BLACK);

    // Draw selected text
    std::string displayText = (selectedIndex >= 0 && selectedIndex < options.size())
                              ? options[selectedIndex] : text;
    DrawText(displayText.c_str(), bounds.x + 5, bounds.y + (bounds.height - 20) / 2, 20, WHITE);

    // Draw dropdown arrow
    DrawTriangle(
        {bounds.x + bounds.width - 15, bounds.y + 10},
        {bounds.x + bounds.width - 5, bounds.y + 10},
        {bounds.x + bounds.width - 10, bounds.y + 20},
        WHITE
    );

    // Draw dropdown list if open
    if (isOpen && !options.empty()) {
        float itemHeight = 30;
        Rectangle listBounds = {
            bounds.x,
            bounds.y + bounds.height,
            bounds.width,
            itemHeight * options.size()
        };

        DrawRectangleRec(listBounds, DARKGRAY);
        DrawRectangleLinesEx(listBounds, 1, BLACK);

        for (int i = 0; i < options.size(); i++) {
            Rectangle itemBounds = {
                bounds.x,
                bounds.y + bounds.height + i * itemHeight,
                bounds.width,
                itemHeight
            };

            // Highlight hovered item
            Vector2 mousePos = GetMousePosition();
            if (CheckCollisionPointRec(mousePos, itemBounds)) {
                DrawRectangleRec(itemBounds, GRAY);
            }

            // Draw item text
            DrawText(options[i].c_str(), itemBounds.x + 5, itemBounds.y + 5, 20, WHITE);
        }
    }
}

bool Dropdown::update() {
    if (!isVisible) return false;

    Vector2 mousePos = GetMousePosition();
    bool wasOpen = isOpen;
    int oldSelection = selectedIndex;

    // Check if main dropdown is clicked
    if (CheckCollisionPointRec(mousePos, bounds)) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isOpen = !isOpen;
        }
    }
    // Check if clicked outside to close
    else if (isOpen && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isOpen = false;
    }

    // Check if an option is selected
    if (isOpen && !options.empty()) {
        float itemHeight = 30;

        for (int i = 0; i < options.size(); i++) {
            Rectangle itemBounds = {
                bounds.x,
                bounds.y + bounds.height + i * itemHeight,
                bounds.width,
                itemHeight
            };

            if (CheckCollisionPointRec(mousePos, itemBounds)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    selectedIndex = i;
                    isOpen = false;
                }
            }
        }
    }

    return wasOpen != isOpen || oldSelection != selectedIndex;
}

// Slider implementation
void Slider::draw() {
    if (!isVisible) return;

    // Draw slider label
    DrawText(text.c_str(), bounds.x, bounds.y - 20, 20, WHITE);

    // Draw slider track
    DrawRectangleRec(bounds, DARKGRAY);
    DrawRectangleLinesEx(bounds, 1, BLACK);

    // Calculate knob position
    float range = maxValue - minValue;
    float knobPos = bounds.x + (value - minValue) / range * bounds.width;

    // Draw slider knob
    Rectangle knobRect = {
        knobPos - 5,
        bounds.y - 5,
        10,
        bounds.height + 10
    };
    DrawRectangleRec(knobRect, LIGHTGRAY);
    DrawRectangleLinesEx(knobRect, 1, BLACK);

    // Draw value
    char valueText[16];
    sprintf(valueText, "%.1f", value);
    DrawText(valueText, bounds.x + bounds.width + 10, bounds.y, 20, WHITE);
}

bool Slider::update() {
    if (!isVisible) return false;

    Vector2 mousePos = GetMousePosition();
    float oldValue = value;

    // Check if slider is being dragged
    Rectangle hitArea = {
        bounds.x - 5,
        bounds.y - 5,
        bounds.width + 10,
        bounds.height + 10
    };

    if (CheckCollisionPointRec(mousePos, hitArea)) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            // Calculate new value based on mouse position
            float range = maxValue - minValue;
            float relativeX = mousePos.x - bounds.x;

            // Clamp relative position
            relativeX = clamp(relativeX, 0.0f, bounds.width);

            // Convert to value
            value = minValue + (relativeX / bounds.width) * range;
        }
    }

    return !(fabs(oldValue - value) < 0.0001f);
}

// NetworkUI implementation
NetworkUI::NetworkUI(NetworkedGameState* gameState)
    : currentState(MAIN_MENU)
    , gameState(gameState)
    , activeElements(nullptr)
    , ipAddressInput(nullptr)
    , portInput(nullptr)
    , playerNameInput(nullptr)
    , chatInput(nullptr)
    , statusLabel(nullptr)
    , playerCountLabel(nullptr)
    , pingLabel(nullptr)
    , enableRollbackCheckbox(nullptr)
    , inputDelaySlider(nullptr)
{
    // Create UI elements for each state
    createMainMenuUI();
    createHostGameUI();
    createJoinGameUI();
    createLobbyUI();
    createMatchOptionsUI();
    createChatUI();

    // Set initial state
    showMainMenu();
}

NetworkUI::~NetworkUI() {
    clearUIContainers();
}

void NetworkUI::update() {
    if (!activeElements) return;

    // Update all active elements
    for (auto& element : *activeElements) {
        if (element) {
            element->update();
        }
    }

    // Update state-specific logic
    switch (currentState) {
        case LOBBY:
            refreshPlayerList();
            refreshNetworkStats();
            break;

        default:
            break;
    }
}

void NetworkUI::draw() {
    if (!activeElements) return;

    // Draw semi-transparent background
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));

    // Draw all active elements
    for (auto& element : *activeElements) {
        if (element) {
            element->draw();
        }
    }

    // Draw state-specific elements
    switch (currentState) {
        case MAIN_MENU:
            DrawText("NETWORK PLAY", GetScreenWidth() / 2 - 150, 100, 40, RAYWHITE);
            break;

        case HOST_GAME:
            DrawText("HOST GAME", GetScreenWidth() / 2 - 100, 100, 40, RAYWHITE);
            break;

        case JOIN_GAME:
            DrawText("JOIN GAME", GetScreenWidth() / 2 - 100, 100, 40, RAYWHITE);
            break;

        case LOBBY:
            DrawText("GAME LOBBY", GetScreenWidth() / 2 - 100, 100, 40, RAYWHITE);
            break;

        case MATCH_OPTIONS:
            DrawText("MATCH OPTIONS", GetScreenWidth() / 2 - 130, 100, 40, RAYWHITE);
            break;

        case CHAT:
            DrawText("CHAT", GetScreenWidth() / 2 - 50, 100, 40, RAYWHITE);

            // Draw chat history
            {
                std::vector<std::string> chatHistory = gameState->getChatHistory();
                int startY = 200;
                int maxMessages = 10;

                for (int i = 0; i < std::min(maxMessages, (int)chatHistory.size()); i++) {
                    int messageIndex = chatHistory.size() - 1 - i;
                    DrawText(chatHistory[messageIndex].c_str(),
                             250, startY + (maxMessages - i - 1) * 25,
                             18, WHITE);
                }
            }
            break;
    }
}

void NetworkUI::showMainMenu() {
    currentState = MAIN_MENU;
    activeElements = &mainMenuElements;
}

void NetworkUI::showHostGameMenu() {
    currentState = HOST_GAME;
    activeElements = &hostGameElements;

    // Set default values
    if (portInput) {
        portInput->setValue("7777");
    }

    if (playerNameInput) {
        NetworkManager& netManager = NetworkManager::getInstance();
        playerNameInput->setValue(netManager.getLocalPlayerName());
    }
}

void NetworkUI::showJoinGameMenu() {
    currentState = JOIN_GAME;
    activeElements = &joinGameElements;

    // Set default values
    if (ipAddressInput) {
        ipAddressInput->setValue("127.0.0.1");
    }

    if (portInput) {
        portInput->setValue("7777");
    }

    if (playerNameInput) {
        NetworkManager& netManager = NetworkManager::getInstance();
        playerNameInput->setValue(netManager.getLocalPlayerName());
    }
}

void NetworkUI::showLobby() {
    currentState = LOBBY;
    activeElements = &lobbyElements;

    refreshPlayerList();
    refreshNetworkStats();
}

void NetworkUI::showMatchOptions() {
    currentState = MATCH_OPTIONS;
    activeElements = &matchOptionsElements;

    // Set values from game state
    if (enableRollbackCheckbox) {
        enableRollbackCheckbox->isChecked = gameState->isRollbackEnabled();
    }

    if (inputDelaySlider) {
        inputDelaySlider->setValue(gameState->getInputDelay());
    }
}

void NetworkUI::showChatUI() {
    currentState = CHAT;
    activeElements = &chatElements;
}

void NetworkUI::onHostGameClicked() {
    // Update player name
    if (playerNameInput) {
        NetworkManager& netManager = NetworkManager::getInstance();
        netManager.setLocalPlayerName(playerNameInput->getValue());
    }

    // Get port number
    int port = 7777;  // Default
    if (portInput && !portInput->getValue().empty()) {
        port = std::stoi(portInput->getValue());
    }

    // Start hosting
    bool success = gameState->hostGame(port);

    if (success) {
        showLobby();
    } else if (statusLabel) {
        statusLabel->setText("Failed to start server");
    }
}

void NetworkUI::onJoinGameClicked() {
    // Update player name
    if (playerNameInput) {
        NetworkManager& netManager = NetworkManager::getInstance();
        netManager.setLocalPlayerName(playerNameInput->getValue());
    }

    // Get IP and port
    std::string ip = "127.0.0.1";  // Default
    if (ipAddressInput) {
        ip = ipAddressInput->getValue();
    }

    int port = 7777;  // Default
    if (portInput && !portInput->getValue().empty()) {
        port = std::stoi(portInput->getValue());
    }

    // Join game
    bool success = gameState->joinGame(ip, port);

    if (success) {
        showLobby();
    } else if (statusLabel) {
        statusLabel->setText("Failed to connect to server");
    }
}

void NetworkUI::onStartMatchClicked() {
    // Set match settings
    if (enableRollbackCheckbox) {
        gameState->setRollbackEnabled(enableRollbackCheckbox->isChecked);
    }

    if (inputDelaySlider) {
        gameState->setInputDelay(static_cast<int>(inputDelaySlider->getValue()));
    }

    // Start the match
    gameState->changeState(GameState::GAME_START);
}

void NetworkUI::onCancelClicked() {
    // Go back to main menu or disconnect
    if (currentState == LOBBY || currentState == MATCH_OPTIONS || currentState == CHAT) {
        gameState->disconnectFromGame();
    }

    showMainMenu();
}

void NetworkUI::onConnectClicked() {
    // Similar to join game but with direct input
    std::string ip = ipAddressInput ? ipAddressInput->getValue() : "127.0.0.1";
    std::string portStr = portInput ? portInput->getValue() : "7777";
    int port = std::stoi(portStr);

    // Update player name
    if (playerNameInput) {
        NetworkManager& netManager = NetworkManager::getInstance();
        netManager.setLocalPlayerName(playerNameInput->getValue());
    }

    // Join game
    bool success = gameState->joinGame(ip, port);

    if (success) {
        showLobby();
    } else if (statusLabel) {
        statusLabel->setText("Failed to connect to server");
    }
}

void NetworkUI::onDisconnectClicked() {
    gameState->disconnectFromGame();
    showMainMenu();
}

void NetworkUI::onSendChatClicked() {
    if (chatInput && !chatInput->getValue().empty()) {
        // Send chat message
        gameState->sendChatMessage(chatInput->getValue());

        // Clear input
        chatInput->setValue("");
    }
}

void NetworkUI::refreshPlayerList() {
    NetworkManager& netManager = NetworkManager::getInstance();

    // Update player count
    if (playerCountLabel) {
        // We don't have direct access to peers, so for now just show 1 (local player)
        // TODO: Add method to NetworkManager to get peer count
        int playerCount = 1;  // Just local player for now
        playerCountLabel->setText("Players: " + std::to_string(playerCount));
    }

    // TODO: Update player labels once we have proper player list API
    // For now, just show the host/client status
    if (statusLabel) {
        if (gameState->getNetworkMode() == NetworkedGameState::HOST) {
            statusLabel->setText("Status: Hosting");
        } else {
            statusLabel->setText("Status: Connected as Client");
        }
    }
}

void NetworkUI::refreshNetworkStats() {
    // Update ping display
    if (pingLabel) {
        int ping = gameState->getAveragePing();

        Color pingColor;
        if (ping < 50) {
            pingColor = GREEN;
        } else if (ping < 100) {
            pingColor = YELLOW;
        } else {
            pingColor = RED;
        }

        pingLabel->setText("Ping: " + std::to_string(ping) + "ms");
        pingLabel->textColor = pingColor;
    }
}

void NetworkUI::createMainMenuUI() {
    // Title is drawn separately

    // Host Game button
    Button* hostButton = new Button(
        {GetScreenWidth() / 2 - 150, 200, 300, 50},
        "Host Game",
        [this]() { this->showHostGameMenu(); }
    );
    mainMenuElements.push_back(hostButton);

    // Join Game button
    Button* joinButton = new Button(
        {GetScreenWidth() / 2 - 150, 270, 300, 50},
        "Join Game",
        [this]() { this->showJoinGameMenu(); }
    );
    mainMenuElements.push_back(joinButton);

    // Back button
    Button* backButton = new Button(
        {GetScreenWidth() / 2 - 100, 400, 200, 50},
        "Back to Main Menu",
        []() { /* Handle in Game.cpp */ }
    );
    mainMenuElements.push_back(backButton);
}

void NetworkUI::createHostGameUI() {
    // Name input
    Label* nameLabel = new Label(
        {GetScreenWidth() / 2 - 200, 180, 200, 30},
        "Your Name:"
    );
    hostGameElements.push_back(nameLabel);

    playerNameInput = new TextInput(
        {GetScreenWidth() / 2 - 200, 210, 300, 40},
        "Enter your name"
    );
    hostGameElements.push_back(playerNameInput);

    // Port input
    Label* portLabel = new Label(
        {GetScreenWidth() / 2 - 200, 270, 200, 30},
        "Port:"
    );
    hostGameElements.push_back(portLabel);

    portInput = new TextInput(
        {GetScreenWidth() / 2 - 200, 300, 150, 40},
        "7777"
    );
    hostGameElements.push_back(portInput);

    // Host button
    Button* hostButton = new Button(
        {GetScreenWidth() / 2 - 150, 370, 300, 50},
        "Start Hosting",
        [this]() { this->onHostGameClicked(); }
    );
    hostGameElements.push_back(hostButton);

    // Cancel button
    Button* cancelButton = new Button(
        {GetScreenWidth() / 2 - 100, 440, 200, 50},
        "Cancel",
        [this]() { this->showMainMenu(); }
    );
    hostGameElements.push_back(cancelButton);
}

void NetworkUI::createJoinGameUI() {
    // Name input
    Label* nameLabel = new Label(
        {GetScreenWidth() / 2 - 200, 160, 200, 30},
        "Your Name:"
    );
    joinGameElements.push_back(nameLabel);

    playerNameInput = new TextInput(
        {GetScreenWidth() / 2 - 200, 190, 300, 40},
        "Enter your name"
    );
    joinGameElements.push_back(playerNameInput);

    // IP input
    Label* ipLabel = new Label(
        {GetScreenWidth() / 2 - 200, 240, 200, 30},
        "Server IP:"
    );
    joinGameElements.push_back(ipLabel);

    ipAddressInput = new TextInput(
        {GetScreenWidth() / 2 - 200, 270, 300, 40},
        "127.0.0.1"
    );
    joinGameElements.push_back(ipAddressInput);

    // Port input
    Label* portLabel = new Label(
        {GetScreenWidth() / 2 - 200, 320, 200, 30},
        "Server Port:"
    );
    joinGameElements.push_back(portLabel);

    portInput = new TextInput(
        {GetScreenWidth() / 2 - 200, 350, 150, 40},
        "7777"
    );
    joinGameElements.push_back(portInput);

    // Connect button
    Button* connectButton = new Button(
        {GetScreenWidth() / 2 - 150, 410, 300, 50},
        "Connect to Server",
        [this]() { this->onConnectClicked(); }
    );
    joinGameElements.push_back(connectButton);

    // Status label for connection errors
    statusLabel = new Label(
        {GetScreenWidth() / 2 - 200, 470, 400, 30},
        "",
        18,
        RED
    );
    joinGameElements.push_back(statusLabel);

    // Cancel button
    Button* cancelButton = new Button(
        {GetScreenWidth() / 2 - 100, 510, 200, 50},
        "Cancel",
        [this]() { this->showMainMenu(); }
    );
    joinGameElements.push_back(cancelButton);
}

void NetworkUI::createLobbyUI() {
    // Status labels
    statusLabel = new Label(
        {GetScreenWidth() / 2 - 200, 160, 400, 30},
        "Status: Connecting...",
        24,
        WHITE
    );
    lobbyElements.push_back(statusLabel);

    playerCountLabel = new Label(
        {GetScreenWidth() / 2 - 200, 190, 200, 30},
        "Players: 1",
        20,
        WHITE
    );
    lobbyElements.push_back(playerCountLabel);

    pingLabel = new Label(
        {GetScreenWidth() / 2 - 200, 220, 200, 30},
        "Ping: --ms",
        20,
        WHITE
    );
    lobbyElements.push_back(pingLabel);

    // Player list area (drawn separately in refresh method)
    Label* playersTitle = new Label(
        {GetScreenWidth() / 2 - 200, 260, 200, 30},
        "Players in Lobby:",
        22,
        WHITE
    );
    lobbyElements.push_back(playersTitle);

    // We'll add player labels dynamically
    for (int i = 0; i < 8; i++) {
        Label* playerLabel = new Label(
            {GetScreenWidth() / 2 - 180, 300 + i * 30, 360, 30},
            "",
            18,
            WHITE
        );
        playerLabel->setVisible(false);
        playerLabels.push_back(playerLabel);
        lobbyElements.push_back(playerLabel);
    }

    // Options button
    Button* optionsButton = new Button(
        {GetScreenWidth() / 2 - 80, 420, 160, 40},
        "Match Options",
        [this]() { this->showMatchOptions(); }
    );
    lobbyElements.push_back(optionsButton);

    // Chat button
    Button* chatButton = new Button(
        {GetScreenWidth() / 2 - 220, 420, 120, 40},
        "Chat",
        [this]() { this->showChatUI(); }
    );
    lobbyElements.push_back(chatButton);

    // Start button (only visible for host)
    Button* startButton = new Button(
        {GetScreenWidth() / 2 + 100, 420, 120, 40},
        "Start Game",
        [this]() { this->onStartMatchClicked(); }
    );
    lobbyElements.push_back(startButton);

    // Disconnect button
    Button* disconnectButton = new Button(
        {GetScreenWidth() / 2 - 100, 480, 200, 50},
        "Disconnect",
        [this]() { this->onDisconnectClicked(); }
    );
    lobbyElements.push_back(disconnectButton);
}

void NetworkUI::createMatchOptionsUI() {
    // Input delay option
    Label* delayLabel = new Label(
        {GetScreenWidth() / 2 - 200, 180, 200, 30},
        "Input Delay:"
    );
    matchOptionsElements.push_back(delayLabel);

    inputDelaySlider = new Slider(
        {GetScreenWidth() / 2 - 150, 220, 300, 20},
        "",
        0,
        10,
        2
    );
    matchOptionsElements.push_back(inputDelaySlider);

    // Rollback option
    enableRollbackCheckbox = new Checkbox(
        {GetScreenWidth() / 2 - 150, 280, 300, 30},
        "Enable Rollback Netcode"
    );
    matchOptionsElements.push_back(enableRollbackCheckbox);

    // Save button
    Button* saveButton = new Button(
        {GetScreenWidth() / 2 - 150, 350, 300, 50},
        "Save Options",
        [this]() {
            // Save options
            if (enableRollbackCheckbox) {
                gameState->setRollbackEnabled(enableRollbackCheckbox->isChecked);
            }

            if (inputDelaySlider) {
                gameState->setInputDelay(static_cast<int>(inputDelaySlider->getValue()));
            }

            this->showLobby();
        }
    );
    matchOptionsElements.push_back(saveButton);

    // Cancel button
    Button* cancelButton = new Button(
        {GetScreenWidth() / 2 - 100, 420, 200, 50},
        "Cancel",
        [this]() { this->showLobby(); }
    );
    matchOptionsElements.push_back(cancelButton);
}

void NetworkUI::createChatUI() {
    // Chat history is drawn separately

    // Chat input
    chatInput = new TextInput(
        {GetScreenWidth() / 2 - 250, 500, 400, 40},
        "Type message here..."
    );
    chatElements.push_back(chatInput);

    // Send button
    Button* sendButton = new Button(
        {GetScreenWidth() / 2 + 160, 500, 100, 40},
        "Send",
        [this]() { this->onSendChatClicked(); }
    );
    chatElements.push_back(sendButton);

    // Back button
    Button* backButton = new Button(
        {GetScreenWidth() / 2 - 100, 560, 200, 40},
        "Back to Lobby",
        [this]() { this->showLobby(); }
    );
    chatElements.push_back(backButton);
}

void NetworkUI::clearUIContainers() {
    // Clean up all UI elements
    auto cleanContainer = [](std::vector<UIElement*>& container) {
        for (auto element : container) {
            delete element;
        }
        container.clear();
    };

    cleanContainer(mainMenuElements);
    cleanContainer(hostGameElements);
    cleanContainer(joinGameElements);
    cleanContainer(lobbyElements);
    cleanContainer(matchOptionsElements);
    cleanContainer(chatElements);

    // Reset pointers
    ipAddressInput = nullptr;
    portInput = nullptr;
    playerNameInput = nullptr;
    chatInput = nullptr;
    statusLabel = nullptr;
    playerCountLabel = nullptr;
    pingLabel = nullptr;
    enableRollbackCheckbox = nullptr;
    inputDelaySlider = nullptr;
    playerLabels.clear();
}