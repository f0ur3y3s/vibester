// NetworkUI.h
#ifndef NETWORK_UI_H
#define NETWORK_UI_H

#include "raylib.h"
#include "NetworkedGameState.h"
#include <string>
#include <vector>
#include <functional>
#include "ParticleSystem.h" // For clamp function

#include "raymath.h"

// UI Element Types
enum class UIElementType {
    LABEL,
    BUTTON,
    TEXT_INPUT,
    CHECKBOX,
    DROPDOWN,
    SLIDER
};

// UI Element Base Class
class UIElement {
public:
    UIElement(Rectangle rect, const std::string& text, UIElementType type)
        : bounds(rect), text(text), type(type), isActive(false), isHovered(false), isVisible(true) {}

    virtual ~UIElement() {}

    virtual void draw() = 0;
    virtual bool update() = 0;  // Returns true if state changed

    void setVisible(bool visible) { isVisible = visible; }
    bool isElementVisible() const { return isVisible; }

    Rectangle bounds;
    std::string text;
    UIElementType type;
    bool isActive;
    bool isHovered;
    bool isVisible;
};

// Button Element
class Button : public UIElement {
public:
    Button(Rectangle rect, const std::string& text, const std::function<void()>& callback)
        : UIElement(rect, text, UIElementType::BUTTON), onClick(callback) {}

    void draw() override;
    bool update() override;

    std::function<void()> onClick;
};

// Label Element
class Label : public UIElement {
public:
    Label(Rectangle rect, const std::string& text, int fontSize = 20, Color color = WHITE)
        : UIElement(rect, text, UIElementType::LABEL), fontSize(fontSize), textColor(color) {}

    void draw() override;
    bool update() override;

    void setText(const std::string& newText) { text = newText; }

    int fontSize;
    Color textColor;
};

// Text Input Element
class TextInput : public UIElement {
public:
    TextInput(Rectangle rect, const std::string& placeholder)
        : UIElement(rect, "", UIElementType::TEXT_INPUT), placeholder(placeholder),
          cursorPos(0), selectionStart(0), isEditing(false) {}

    void draw() override;
    bool update() override;

    std::string getValue() const { return text; }
    void setValue(const std::string& value) { text = value; cursorPos = text.length(); }

    std::string placeholder;
    int cursorPos;
    int selectionStart;
    bool isEditing;
};

// Checkbox Element
class Checkbox : public UIElement {
public:
    Checkbox(Rectangle rect, const std::string& text, bool checked = false)
        : UIElement(rect, text, UIElementType::CHECKBOX), isChecked(checked) {}

    void draw() override;
    bool update() override;

    bool isChecked;
};

// Dropdown Element
class Dropdown : public UIElement {
public:
    Dropdown(Rectangle rect, const std::string& text, const std::vector<std::string>& options)
        : UIElement(rect, text, UIElementType::DROPDOWN), options(options),
          selectedIndex(0), isOpen(false) {}

    void draw() override;
    bool update() override;

    int getSelectedIndex() const { return selectedIndex; }
    std::string getSelectedValue() const {
        return (selectedIndex >= 0 && selectedIndex < options.size()) ? options[selectedIndex] : "";
    }
    void setOptions(const std::vector<std::string>& newOptions) {
        options = newOptions;
        if (selectedIndex >= options.size()) selectedIndex = options.empty() ? -1 : 0;
    }

    std::vector<std::string> options;
    int selectedIndex;
    bool isOpen;
};

// Slider Element
class Slider : public UIElement {
public:
    Slider(Rectangle rect, const std::string& text, float min, float max, float value)
        : UIElement(rect, text, UIElementType::SLIDER),
          minValue(min), maxValue(max), value(value) {}

    void draw() override;
    bool update() override;

    float getValue() const { return value; }
    void setValue(float newValue) { value = clamp(newValue, minValue, maxValue); }

    float minValue;
    float maxValue;
    float value;
};

// Network Menu UI Manager
class NetworkUI {
public:
    NetworkUI(NetworkedGameState* gameState);
    ~NetworkUI();

    // UI States
    enum UIState {
        MAIN_MENU,
        HOST_GAME,
        JOIN_GAME,
        LOBBY,
        MATCH_OPTIONS,
        CHAT
    };

    // Update and draw
    void update();
    void draw();

    // Navigation
    void showMainMenu();
    void showHostGameMenu();
    void showJoinGameMenu();
    void showLobby();
    void showMatchOptions();
    void showChatUI();

    // Event handlers
    void onHostGameClicked();
    void onJoinGameClicked();
    void onStartMatchClicked();
    void onCancelClicked();
    void onConnectClicked();
    void onDisconnectClicked();
    void onSendChatClicked();

    // State accessors
    UIState getCurrentState() const { return currentState; }

private:
    // UI state
    UIState currentState;
    NetworkedGameState* gameState;

    // UI containers for each state
    std::vector<UIElement*> mainMenuElements;
    std::vector<UIElement*> hostGameElements;
    std::vector<UIElement*> joinGameElements;
    std::vector<UIElement*> lobbyElements;
    std::vector<UIElement*> matchOptionsElements;
    std::vector<UIElement*> chatElements;

    // Active UI container reference
    std::vector<UIElement*>* activeElements;

    // Input fields
    TextInput* ipAddressInput;
    TextInput* portInput;
    TextInput* playerNameInput;
    TextInput* chatInput;

    // Status indicators
    Label* statusLabel;
    Label* playerCountLabel;
    Label* pingLabel;

    // Options
    Checkbox* enableRollbackCheckbox;
    Slider* inputDelaySlider;

    // Player list in lobby
    std::vector<Label*> playerLabels;

    // Helper methods
    void createMainMenuUI();
    void createHostGameUI();
    void createJoinGameUI();
    void createLobbyUI();
    void createMatchOptionsUI();
    void createChatUI();

    void refreshPlayerList();
    void refreshNetworkStats();
    void clearUIContainers();
};

#endif // NETWORK_UI_H