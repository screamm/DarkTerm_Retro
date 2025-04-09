#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <json/json.h> // Använder jsoncpp för parsing (CMake måste hitta detta)

class ThemeManager {
public:
    struct Color {
        float r, g, b;

        Color() : r(0.0f), g(0.0f), b(0.0f) {}
        Color(float r_, float g_, float b_) : r(r_), g(g_), b(b_) {}

        // Konvertera från hex-sträng (#RRGGBB)
        static Color fromHex(const std::string& hex);
        // Konvertera till hex-sträng
        std::string toHex() const;
    };

    struct Theme {
        std::string name = "Default";
        std::string description = "Default theme";
        std::map<int, Color> palette;
        int bgColor = 0;
        int fgColor = 7;
        int cursorColor = 15;
        float scanlineIntensity = 0.2f; // För CRT-shader
        float curvature = 0.1f;         // För CRT-shader
        float rgbShift = 0.001f;        // För CRT-shader

        // Default-konstruktor som initierar med standardpalett
        Theme();
    };

private:
    std::map<std::string, Theme> themes;
    std::string currentThemeName;

    // Lägg till fördefinierade teman
    void addBuiltInThemes();

public:
    ThemeManager();

    // Hämta nuvarande tema
    const Theme& getCurrentTheme() const;

    // Sätt nuvarande tema via namn
    bool setTheme(const std::string& themeName);

    // Hämta lista med tillgängliga teman
    std::vector<std::string> getThemeNames() const;

    // Ladda tema från JSON-fil
    bool loadThemeFromFile(const std::string& filename);

    // Hämta ett specifikt tema (används internt eller för förhandsvisning)
    const Theme* getTheme(const std::string& themeName) const;
};

#endif // THEME_MANAGER_H
