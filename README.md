# Super Smash Clone

A 2D platform fighting game inspired by Super Smash Bros, built with Raylib.

![Game Screenshot](screenshot.png)

## Table of Contents

- [Overview](#overview)
- [Building the Project](#building-the-project)
  - [Prerequisites](#prerequisites)
  - [Build Instructions](#build-instructions)
  - [Troubleshooting](#troubleshooting)
- [Game Controls](#game-controls)
  - [Basic Movement](#basic-movement)
  - [Attack System](#attack-system)
  - [Defensive Options](#defensive-options)
  - [Advanced Techniques](#advanced-techniques)
- [Game Mechanics](#game-mechanics)
  - [Platform Types](#platform-types)
  - [Damage and Knockback System](#damage-and-knockback-system)
  - [Character States](#character-states)
- [AI System](#ai-system)
- [Development Notes](#development-notes)
- [License](#license)

## Overview

Super Smash Clone is a platform fighter that replicates the core mechanics of Super Smash Bros. The game features:

- Smash-style movement and physics
- Percentage-based damage system
- Directional attacks and knockback
- Stock-based lives system
- Platform collision with pass-through platforms
- Advanced AI opponent with multiple difficulty levels

## Building the Project

### Prerequisites

- C++11 compatible compiler (GCC, Clang, MSVC)
- CMake 3.10 or higher
- Git
- Raylib dependencies (automatically handled by CMake)

#### Linux Dependencies

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential git cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev

# Fedora
sudo dnf install gcc-c++ git cmake alsa-lib-devel libX11-devel libXrandr-devel libXi-devel mesa-libGL-devel mesa-libGLU-devel libXcursor-devel libXinerama-devel
```

#### macOS Dependencies

```bash
# Using Homebrew
brew install cmake
```

#### Windows Dependencies

- Visual Studio with C++ development workload
- CMake (can be installed through Visual Studio)
- Git for Windows

### Build Instructions

#### Clone the Repository

```bash
git clone https://github.com/yourusername/super-smash-clone.git
cd super-smash-clone
```

#### CMake Build (all platforms)

```bash
mkdir build
cd build
cmake ..
```

#### Linux/macOS Build

```bash
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # macOS
```

#### Windows Build

```bash
# Using Visual Studio
cmake --build . --config Release

# Or open the generated .sln file in Visual Studio and build
```

#### Run the Game

```bash
# From the build directory
./test_toilet  # Linux/macOS
.\Debug\test_toilet.exe  # Windows (Debug build)
.\Release\test_toilet.exe  # Windows (Release build)
```

### Troubleshooting

#### Common Build Issues

1. **Missing Raylib dependencies**
  - CMake should handle downloading Raylib, but if you encounter errors, try installing Raylib manually:
   ```bash
   git clone https://github.com/raysan5/raylib.git
   cd raylib/src
   make PLATFORM=PLATFORM_DESKTOP
   sudo make install
   ```

2. **fmin/std::min error**
  - If you get errors about `fmin` not being declared, ensure you've included `<algorithm>` and use `std::min` instead.

3. **Linker errors**
  - On Linux, you might need additional libraries: `-lGL -lm -lpthread -ldl -lrt -lX11`

4. **CMake not finding Raylib**
  - Set Raylib's installation path manually:
   ```bash
   cmake .. -DCMAKE_PREFIX_PATH=/path/to/raylib/installation
   ```

## Game Controls

### Basic Movement

| Action           | Key                   |
|------------------|------------------------|
| Movement         | WASD                   |
| Left             | A                      |
| Right            | D                      |
| Jump             | W                      |
| Fast Fall        | S (in air)             |
| Drop Through     | S (on platform)        |
| Face Left/Right  | (Automatic based on movement) |

### Attack System

| Action           | Key                   |
|------------------|------------------------|
| Basic Attack     | J                      |
| Special Attack   | K                      |
| Smash Attack     | L                      |
| Grab             | U                      |

#### Directional Attacks

Basic attacks (J) while holding a direction:
- Neutral: J alone
- Forward: J + D (or J + A if facing right)
- Up: J + W
- Down: J + S

#### Aerial Attacks

When in the air, the attack controls change:
- Neutral Air: J alone
- Forward Air: J + D (or J + A if facing right)
- Back Air: J + A (or J + D if facing right)
- Up Air: J + W
- Down Air: J + S

#### Special Attacks

Special attacks (K) while holding a direction:
- Neutral: K alone
- Side: K + D/A
- Up: K + W
- Down: K + S

#### Smash Attacks

Smash attacks (L) while holding a direction:
- Forward: L + D/A
- Up: L + W
- Down: L + S

#### Grab and Throws

- Grab: U
- Pummel: J (while grabbing)
- Forward Throw: D (while grabbing)
- Back Throw: A (while grabbing)
- Up Throw: W (while grabbing)
- Down Throw: S (while grabbing)

### Defensive Options

| Action           | Key                   |
|------------------|------------------------|
| Shield           | I (hold)               |
| Spot Dodge       | I + S                  |
| Forward Roll     | I + D                  |
| Backward Roll    | I + A                  |
| Air Dodge        | I (in air)             |

### Advanced Techniques

| Technique        | Input                 |
|------------------|------------------------|
| Fast Fall        | S (at peak of jump)    |
| Platform Drop    | S (while on platform)  |
| Short Hop        | Tap W quickly          |
| Dash Attack      | J during run           |
| Directional Air Dodge | I + Direction (in air) |
| Shield Cancel    | Release I              |
| Shield Grab      | U while shielding      |

## Game Mechanics

### Platform Types

The game features two types of platforms:

1. **Solid Platforms (SOLID)**
  - Collision from all directions
  - Cannot pass through from any side
  - Typically used for main stage platforms and walls

2. **Pass-Through Platforms (PASSTHROUGH)**
  - Collision only from above
  - Can jump up through from below
  - Can move horizontally through
  - Can drop down by pressing S
  - Highlighted only on top edge for visual distinction

### Damage and Knockback System

Similar to Smash Bros, the game uses a percentage-based damage system:

- Characters accumulate damage as a percentage (0% to 999%)
- Higher damage leads to greater knockback when hit
- Knockback formula: `baseKnockback + (damage * damagePercent * DAMAGE_SCALING * knockbackScaling)`
- Characters are KO'd when knocked beyond the blast zones

### Character States

Characters can be in various states that affect their available actions:

- IDLE: Standing still
- RUNNING: Moving horizontally on ground
- JUMPING: Moving upward in the air
- FALLING: Moving downward in the air
- ATTACKING: Performing an attack (cannot be interrupted)
- SHIELDING: Blocking attacks
- DODGING: Invincible roll/spot dodge
- HITSTUN: Being hit and unable to act
- DYING: Death animation after being KO'd

## AI System

The game features an enhanced AI opponent with different difficulty levels:

| Difficulty Level | Key         | Description                                      |
|------------------|-------------|--------------------------------------------------|
| Easy             | 1           | Slower reactions, makes frequent mistakes        |
| Medium           | 2           | Balanced AI with moderate skill                  |
| Hard             | 3           | Quick reactions, smarter decision making         |
| Expert           | 4           | Tournament-level AI with advanced techniques     |

The AI features:
- Adaptive behavior that responds to player patterns
- Multiple combat states (neutral, approach, attack, defend, etc.)
- Risk/reward decision making
- Combo recognition and execution
- Edge guarding and recovery strategies

## Development Notes

- Built using Raylib for rendering and input
- Custom physics system for platform fighting mechanics
- Character state machine for managing actions and animations
- Hitbox/hurtbox system for precise collision detection
- AI system based on utility-based decision making

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Inspired by Nintendo's Super Smash Bros series
- Raylib library by Ramon Santamaria
- Contributors and testers