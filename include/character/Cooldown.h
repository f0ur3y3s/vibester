#ifndef CHARACTER_COOLDOWN_H
#define CHARACTER_COOLDOWN_H

// Simple cooldown timer class
class Cooldown {
public:
    int duration;
    int current;
    
    Cooldown(int dur) : duration(dur), current(0) {}
    
    bool isActive() const { return current > 0; }
    void update() { if (current > 0) current--; }
    void reset() { current = duration; }
    void reset(float multiplier) { current = static_cast<int>(duration * multiplier); }
};

#endif // CHARACTER_COOLDOWN_H