# DarkTerm - En Retro Terminalemulator

DarkTerm är ett försök att skapa en enkel, retro-inspirerad terminalemulator med hjälp av C++ och OpenGL.

## Funktioner (Planerade/Under utveckling)

*   Rendering av tecken med FreeType
*   Stöd för grundläggande ANSI escape-koder (färger, markörpositionering)
*   Konfigurerbara färgteman (via JSON)
*   Valfri CRT-skärmeffekt (scanlines, curvatur)
*   Blinkande markör

## Beroenden

För att bygga och köra DarkTerm behöver du:

*   **CMake** (minst version 3.10)
*   En C++-kompilator med stöd för C++17 (t.ex. GCC, Clang, MSVC)
*   **GLFW** (Bibliotek för fönsterhantering och input)
*   **FreeType** (Bibliotek för fontrendering)
*   **JsonCpp** (Bibliotek för att läsa JSON-konfigurationsfiler)
*   **GLAD** (OpenGL Loading Library - inkluderas som submodule eller via `FetchContent`)

På **macOS** kan du installera de flesta beroenden med Homebrew:
```bash
brew install cmake glfw freetype jsoncpp
```

På **Linux** (Debian/Ubuntu):
```bash
sudo apt update
sudo apt install cmake build-essential libglfw3-dev libfreetype-dev libjsoncpp-dev
```

## Bygga Projektet

1.  Klona repot (om det ligger på t.ex. GitHub):
    ```bash
    git clone <repo-url>
    cd DarkTerm
    ```
2.  Skapa en byggkatalog och kör CMake:
    ```bash
    cmake -S . -B build
    ```
3.  Bygg projektet:
    ```bash
    cmake --build build
    ```

## Köra Projektet

Efter en lyckad byggprocess finns den körbara filen i `build`-katalogen:

```bash
./build/DarkTerm
```

## Konfiguration

Färgteman kan definieras i JSON-filer i `themes`-katalogen och väljas i koden (just nu hårdkodat i `ThemeManager.cpp`). Fontfilen som används laddas från `fonts`-katalogen och specificeras i `src/main.cpp`.

## Kända Problem / Att Göra

*   Markören ritas inte korrekt (pågående felsökning).
*   Implementering av fler ANSI escape-koder behövs.
*   CRT-effekten är inte fullt implementerad/testad.
*   Felhantering kan förbättras. 