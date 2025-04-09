# DarkTerm - A Retro Terminal Emulator

DarkTerm is an attempt to create a simple, retro-inspired terminal emulator using C++ and OpenGL.

## Features (Planned/Under Development)

*   Character rendering with FreeType
*   Support for basic ANSI escape codes (colors, cursor positioning)
*   Configurable color themes (via JSON)
*   Optional CRT screen effects (scanlines, curvature)
*   Blinking cursor

## Dependencies

To build and run DarkTerm, you need:

*   **CMake** (minimum version 3.10)
*   A C++ compiler with C++17 support (e.g., GCC, Clang, MSVC)
*   **GLFW** (Library for window management and input)
*   **FreeType** (Library for font rendering)
*   **JsonCpp** (Library for reading JSON configuration files)
*   **GLAD** (OpenGL Loading Library - included as submodule or via `FetchContent`)

On **macOS**, you can install most dependencies with Homebrew:
```bash
brew install cmake glfw freetype jsoncpp
```

On **Linux** (Debian/Ubuntu):
```bash
sudo apt update
sudo apt install cmake build-essential libglfw3-dev libfreetype-dev libjsoncpp-dev
```

## Building the Project

1.  Clone the repository (if it's on GitHub, for example):
    ```bash
    git clone <repo-url>
    cd DarkTerm
    ```
2.  Create a build directory and run CMake:
    ```bash
    cmake -S . -B build
    ```
3.  Build the project:
    ```bash
    cmake --build build
    ```

## Running the Project

After a successful build process, the executable file is in the `build` directory:

```bash
./build/DarkTerm
```

## Configuration

Color themes can be defined in JSON files in the `themes` directory and selected in the code (currently hardcoded in `ThemeManager.cpp`). The font file used is loaded from the `fonts` directory and specified in `src/main.cpp`.

## Known Issues / To Do

*   Cursor is not rendering correctly (ongoing debugging).
*   Implementation of more ANSI escape codes needed.
*   CRT effect is not fully implemented/tested.
*   Error handling can be improved. 