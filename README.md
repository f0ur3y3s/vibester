# Vibester - A Super Smash Bros Style PvE Fighting Game

Vibester is a 2D platform fighting game inspired by Super Smash Bros, built with C++ and Raylib. Play against an AI opponent in this fast-paced combat game.

## Build Instructions

### Prerequisites

- CMake (3.0 or higher)
- C++ compiler with C++11 support
- Raylib dependencies (handled automatically by CMake)

### Building the Game

#### Using CLion:

1. Open the project in CLion
2. Click on the build button or use the keyboard shortcut (Ctrl+F9)
3. Run the executable from the Run menu or use the keyboard shortcut (Shift+F10)

#### Using Command Line:

```bash
# Clone the repository
git clone https://github.com/your-username/vibester.git
cd vibester

# Create and navigate to build directory
mkdir -p build
cd build

# Generate build files with CMake
cmake ..

# Build the project
make

# Run the game
./test_toilet
```

#### Using Snap CLion's CMake:

If you're using the Snap version of CLion:

```bash
/snap/clion/332/bin/cmake/linux/x64/bin/cmake --build /path/to/vibester/cmake-build-debug --target test_toilet -j 38
```

## Game Controls

### Player Controls:

#### Movement:
- **W**: Jump
- **A**: Move left
- **D**: Move right
- **S**: Fast fall

#### Standard Attacks:
- **J**: Jab/Neutral attack (context-sensitive)
- **K + direction**: Special attack
  - **K (neutral)**: Neutral special
  - **K + W**: Up special (recovery)
  - **K + A/D**: Side special
  - **K + S**: Down special

#### Smash Attacks:
- **L + direction**: Smash attack
  - **L + W**: Up smash
  - **L + A/D**: Forward smash
  - **L + S**: Down smash

#### Defense:
- **I**: Shield
- **I + direction**: Dodge
  - **I + A**: Dodge left
  - **I + D**: Dodge right
  - **I + S**: Spot dodge

#### Grabs:
- **U**: Grab
- When grabbing:
  - **J**: Pummel
  - **W**: Up throw
  - **A**: Back throw
  - **D**: Forward throw
  - **S**: Down throw

### Enemy AI:

The game features an AI-controlled enemy that:
- Tracks your position and moves accordingly
- Uses a variety of attacks based on distance
- Performs recovery moves when knocked off stage
- Uses shields and dodges to avoid attacks
- Makes combat decisions with some randomness for unpredictability

### Other Controls:

- **P** or **ESC**: Pause game
- **R** (when paused): Restart game
- **F1**: Toggle debug mode
- **Enter/Space**: Confirm/Continue at title and results screens

## Game Mechanics

### Core Mechanics:

- **Damage System**: Characters accumulate damage percentage. Higher damage means more knockback when hit.
- **Knockback**: Characters are knocked back based on damage percentage, attack power, and angle.
- **Stock System**: Each player has a set number of lives (stocks).
- **Blast Zones**: Getting knocked out of the screen's blast zones leads to losing a stock.

### Advanced Mechanics:

- **Double Jump**: Characters can jump a second time in the air.
- **Fast Fall**: Press down while falling to increase falling speed.
- **Shielding**: Reduces damage and knockback but deteriorates with use.
- **Dodging**: Brief invincibility frames to avoid attacks.
- **Grabs and Throws**: Bypass shields and offer different knockback options.
- **Air Attacks**: Different attacks can be performed while airborne.
- **Smash Attacks**: Stronger versions of standard attacks.
- **Special Attacks**: Unique abilities including projectiles and recovery moves.

## Architecture Overview

- **Character System**: Handles character states, attacks, physics, and collision
- **Platform System**: Creates the stage with collision detection
- **Attack System**: Manages hitboxes, damage, and knockback calculations
- **Particle System**: Visual effects for hits, explosions, and movement
- **Game State Management**: Controls game flow between menu, character selection, play, and results
- **Input Handling**: Processes player inputs and translates them to character actions

## Technical Details

### Key Classes:

- **Character**: Manages character state, movement, attacks, and collision
- **AttackBox**: Handles hitboxes, damage calculations, and knockback
- **Platform**: Provides collision surfaces for characters
- **Particle/ParticleSystem**: Visual effects for impacts and deaths
- **GameState**: Controls game flow and state transitions
- **Item**: (Future implementation) Power-ups and weapons

### Files Overview:

- **main.cpp**: Original entry point, now renamed to avoid conflicts
- **Game.cpp**: Current main entry point with game loop and core logic 
- **Character.h/cpp**: Character class implementation
- **AttackBox.h/cpp**: Hitbox and attack system
- **Platform.h/cpp**: Stage platforms
- **Particle.h/cpp**: Individual particle effects
- **ParticleSystem.h/cpp**: Particle generation and management
- **GameState.h/cpp**: Game state management
- **Item.h**: Item system (partial implementation)
- **Constants.h**: Game constants and configuration
- **Stage.h**: Stage definitions (not fully implemented)

## Troubleshooting

If you encounter build errors:

1. **Multiple main definitions**: Make sure you're using the latest code where this issue is fixed
2. **CMake not found**: Verify CMake is installed or use the CLion embedded version
3. **Missing Raylib**: The FetchContent in CMake should automatically download Raylib
4. **Compiler errors**: Ensure you have a C++11 compatible compiler

## Future Development

- Complete item system implementation
- Add more character types with unique movesets
- Implement stage hazards and interactive elements
- Add sound effects and music
- Create more particle effects and visual polish
- Implement a proper character selection screen

## Credits

Built with [Raylib](https://www.raylib.com/) - a simple and easy-to-use library for game development.