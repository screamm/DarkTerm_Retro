#include "ThemeManager.h"
#include <fstream>
#include <cstdio> // För sscanf/sprintf
#include <stdexcept> // För std::out_of_range

// --- Implementering av ThemeManager::Color ---

ThemeManager::Color ThemeManager::Color::fromHex(const std::string& hex) {
    if (hex.length() < 7 || hex[0] != '#') {
        return Color(0.0f, 0.0f, 0.0f); // Returnera svart vid fel
    }
    unsigned int r, g, b;
    // Använd %02x för att läsa två hexadecimala siffror
    if (sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3) {
        return Color(r / 255.0f, g / 255.0f, b / 255.0f);
    }
    return Color(0.0f, 0.0f, 0.0f); // Returnera svart vid fel
}

std::string ThemeManager::Color::toHex() const {
    char buffer[8];
    // Använd %02X för att skriva två stora hexadecimala siffror
    sprintf(buffer, "#%02X%02X%02X",
            static_cast<int>(std::max(0.0f, std::min(1.0f, r)) * 255),
            static_cast<int>(std::max(0.0f, std::min(1.0f, g)) * 255),
            static_cast<int>(std::max(0.0f, std::min(1.0f, b)) * 255));
    return std::string(buffer);
}

// --- Implementering av ThemeManager::Theme (Default Constructor) ---
ThemeManager::Theme::Theme() {
    // Initiera med standard 8-bitars palett
    palette = {
        {0, Color(0.0f, 0.0f, 0.0f)},        // Black
        {1, Color(0.5f, 0.0f, 0.0f)},        // Dark Red
        {2, Color(0.0f, 0.5f, 0.0f)},        // Dark Green
        {3, Color(0.5f, 0.5f, 0.0f)},        // Dark Yellow / Brown
        {4, Color(0.0f, 0.0f, 0.5f)},        // Dark Blue
        {5, Color(0.5f, 0.0f, 0.5f)},        // Dark Magenta
        {6, Color(0.0f, 0.5f, 0.5f)},        // Dark Cyan
        {7, Color(0.75f, 0.75f, 0.75f)},     // Light Gray
        {8, Color(0.5f, 0.5f, 0.5f)},        // Dark Gray
        {9, Color(1.0f, 0.0f, 0.0f)},        // Bright Red
        {10, Color(0.0f, 1.0f, 0.0f)},       // Bright Green
        {11, Color(1.0f, 1.0f, 0.0f)},       // Bright Yellow
        {12, Color(0.0f, 0.0f, 1.0f)},       // Bright Blue
        {13, Color(1.0f, 0.0f, 1.0f)},       // Bright Magenta
        {14, Color(0.0f, 1.0f, 1.0f)},       // Bright Cyan
        {15, Color(1.0f, 1.0f, 1.0f)}        // White
    };
}

// --- Implementering av ThemeManager ---

ThemeManager::ThemeManager() {
    // Initiera med inbyggda teman
    addBuiltInThemes();
    // Försök sätta default-tema, fallback till första om det misslyckas
    if (!setTheme("Dark Retro")) {
        if (!themes.empty()) {
             currentThemeName = themes.begin()->first;
        }
    }
}

void ThemeManager::addBuiltInThemes() {
    // Dark Retro Theme (Default)
    {
        Theme darkRetro;
        darkRetro.name = "Dark Retro";
        darkRetro.description = "Classic dark terminal with retro colors";
        darkRetro.bgColor = 0;    // Black background
        darkRetro.fgColor = 7;    // Light gray text
        darkRetro.cursorColor = 15; // White cursor
        darkRetro.scanlineIntensity = 0.15f;
        darkRetro.curvature = 0.1f;
        darkRetro.rgbShift = 0.001f;
        darkRetro.palette[0] = Color(0.05f, 0.05f, 0.1f);  // Nearly black with slight blue tint
        themes[darkRetro.name] = darkRetro;
    }

    // Amber Monochrome
    {
        Theme amber;
        amber.name = "Amber Monochrome";
        amber.description = "Classic amber monitor look";
        amber.bgColor = 0;    // Black background
        amber.fgColor = 3;    // Amber text (index i paletten)
        amber.cursorColor = 11; // Bright amber cursor (index i paletten)
        amber.scanlineIntensity = 0.2f;
        amber.curvature = 0.15f;
        amber.rgbShift = 0.0f;

        // Override palette for monochrome amber look
        for (int i = 0; i < 16; i++) {
            float intensity = (i < 8) ? (i / 7.0f * 0.6f + 0.4f) : ((i - 8) / 7.0f * 0.5f + 0.5f);
             intensity = std::min(1.0f, std::max(0.1f, intensity)); // Clamp intensity
             // Basfärgen är amber (gul-orange)
            amber.palette[i] = Color(1.0f * intensity, 0.65f * intensity, 0.0f * intensity);
        }
        amber.palette[0] = Color(0.1f, 0.065f, 0.0f); // Darker background
        themes[amber.name] = amber;
    }

    // Green Monochrome
     {
        Theme green;
        green.name = "Green Monochrome";
        green.description = "Classic green phosphor monitor look";
        green.bgColor = 0;    // Black background
        green.fgColor = 2;    // Green text
        green.cursorColor = 10; // Bright green cursor
        green.scanlineIntensity = 0.2f;
        green.curvature = 0.15f;
        green.rgbShift = 0.0f;

        // Override palette for monochrome green look
        for (int i = 0; i < 16; i++) {
            float intensity = (i < 8) ? (i / 7.0f * 0.7f + 0.3f) : ((i - 8) / 7.0f * 0.6f + 0.4f);
            intensity = std::min(1.0f, std::max(0.1f, intensity)); // Clamp intensity
            green.palette[i] = Color(0.0f, 1.0f * intensity, 0.1f * intensity); // Lite blått i det gröna
        }
         green.palette[0] = Color(0.0f, 0.1f, 0.02f); // Darker green background
        themes[green.name] = green;
    }

    // IBM CGA Theme
    {
        Theme cga;
        cga.name = "IBM CGA";
        cga.description = "Classic IBM CGA color palette";
        cga.bgColor = 0;    // Index för svart i CGA-paletten nedan
        cga.fgColor = 7;    // Index för ljusgrå
        cga.cursorColor = 15; // Index för vit
        cga.scanlineIntensity = 0.1f;
        cga.curvature = 0.08f;
        cga.rgbShift = 0.0005f;

        // CGA Palette (Mode 1)
        cga.palette[0] = Color(0.0f, 0.0f, 0.0f);        // Black
        cga.palette[1] = Color::fromHex("#0000AA");       // Blue
        cga.palette[2] = Color::fromHex("#00AA00");       // Green
        cga.palette[3] = Color::fromHex("#00AAAA");      // Cyan
        cga.palette[4] = Color::fromHex("#AA0000");       // Red
        cga.palette[5] = Color::fromHex("#AA00AA");      // Magenta
        cga.palette[6] = Color::fromHex("#AA5500");      // Brown (Dark Yellow)
        cga.palette[7] = Color::fromHex("#AAAAAA");     // Light Gray
        cga.palette[8] = Color::fromHex("#555555");     // Dark Gray
        cga.palette[9] = Color::fromHex("#5555FF");      // Bright Blue
        cga.palette[10] = Color::fromHex("#55FF55");     // Bright Green
        cga.palette[11] = Color::fromHex("#55FFFF");      // Bright Cyan
        cga.palette[12] = Color::fromHex("#FF5555");     // Bright Red
        cga.palette[13] = Color::fromHex("#FF55FF");      // Bright Magenta
        cga.palette[14] = Color::fromHex("#FFFF55");      // Bright Yellow
        cga.palette[15] = Color::fromHex("#FFFFFF");       // White
        themes[cga.name] = cga;
    }

    // Modern Dark
    {
        Theme modern;
        modern.name = "Modern Dark";
        modern.description = "A modern dark theme with vibrant colors";
        modern.bgColor = 0;    // Index for dark gray below
        modern.fgColor = 7;    // Index for light gray below
        modern.cursorColor = 14; // Index for bright cyan below
        modern.scanlineIntensity = 0.0f;
        modern.curvature = 0.0f;
        modern.rgbShift = 0.0f;

        // Modern color palette (like a typical Linux terminal theme)
        modern.palette[0] = Color::fromHex("#1E1E1E");  // Dark gray
        modern.palette[1] = Color::fromHex("#CD5C5C");   // Soft red (IndianRed)
        modern.palette[2] = Color::fromHex("#90EE90");  // Soft green (LightGreen)
        modern.palette[3] = Color::fromHex("#F0E68C");  // Soft yellow (Khaki)
        modern.palette[4] = Color::fromHex("#87CEEB");  // Soft blue (SkyBlue)
        modern.palette[5] = Color::fromHex("#DA70D6");  // Soft magenta (Orchid)
        modern.palette[6] = Color::fromHex("#AFEEEE");  // Soft cyan (PaleTurquoise)
        modern.palette[7] = Color::fromHex("#D3D3D3");  // Light gray
        modern.palette[8] = Color::fromHex("#808080");     // Medium gray
        modern.palette[9] = Color::fromHex("#FF6347");     // Bright red (Tomato)
        modern.palette[10] = Color::fromHex("#32CD32");  // Bright green (LimeGreen)
        modern.palette[11] = Color::fromHex("#FFFFE0");   // Bright yellow (LightYellow)
        modern.palette[12] = Color::fromHex("#ADD8E6");  // Bright blue (LightBlue)
        modern.palette[13] = Color::fromHex("#EE82EE");   // Bright magenta (Violet)
        modern.palette[14] = Color::fromHex("#E0FFFF");   // Bright cyan (LightCyan)
        modern.palette[15] = Color::fromHex("#FFFFFF");    // White
        themes[modern.name] = modern;
    }
}

const ThemeManager::Theme& ThemeManager::getCurrentTheme() const {
    try {
        return themes.at(currentThemeName);
    } catch (const std::out_of_range& oor) {
        // Fallback if currentThemeName is somehow invalid (should not happen after constructor)
        if (!themes.empty()) {
            return themes.begin()->second;
        }
        // Should absolutely not happen, but return a default theme if map is empty
        static Theme default_fallback;
        return default_fallback;
    }
}

const ThemeManager::Theme* ThemeManager::getTheme(const std::string& themeName) const {
     auto it = themes.find(themeName);
    if (it != themes.end()) {
        return &(it->second);
    }
    return nullptr; // Theme not found
}

bool ThemeManager::setTheme(const std::string& themeName) {
    if (themes.count(themeName)) {
        currentThemeName = themeName;
        return true;
    }
    return false; // Theme not found
}

std::vector<std::string> ThemeManager::getThemeNames() const {
    std::vector<std::string> names;
    names.reserve(themes.size());
    for (const auto& pair : themes) {
        names.push_back(pair.first);
    }
    return names;
}

bool ThemeManager::loadThemeFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        // Kanske logga ett fel här
        return false;
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs)) {
        // Kanske logga JSON-parsningsfel (errs)
        return false;
    }

    try {
        Theme theme; // Starta med default-tema som bas
        if (!root.isMember("name") || !root["name"].isString()) return false;
        theme.name = root["name"].asString();

        if (root.isMember("description") && root["description"].isString())
            theme.description = root["description"].asString();
        if (root.isMember("bgColor") && root["bgColor"].isInt())
            theme.bgColor = root["bgColor"].asInt();
        if (root.isMember("fgColor") && root["fgColor"].isInt())
            theme.fgColor = root["fgColor"].asInt();
        if (root.isMember("cursorColor") && root["cursorColor"].isInt())
            theme.cursorColor = root["cursorColor"].asInt();
        if (root.isMember("scanlineIntensity") && root["scanlineIntensity"].isNumeric())
            theme.scanlineIntensity = root["scanlineIntensity"].asFloat();
        if (root.isMember("curvature") && root["curvature"].isNumeric())
            theme.curvature = root["curvature"].asFloat();
        if (root.isMember("rgbShift") && root["rgbShift"].isNumeric())
            theme.rgbShift = root["rgbShift"].asFloat();

        // Läs in paletten om den finns
        if (root.isMember("palette") && root["palette"].isObject()) {
            for (int i = 0; i < 16; ++i) {
                std::string colorKey = "color" + std::to_string(i);
                if (root["palette"].isMember(colorKey) && root["palette"][colorKey].isString()) {
                    theme.palette[i] = Color::fromHex(root["palette"][colorKey].asString());
                } // Annars behålls default-värdet för den färgen
            }
        }

        // Lägg till eller ersätt temat
        themes[theme.name] = theme;
        return true;

    } catch (const Json::Exception& e) {
        // Logga JSON-exception
        return false;
    }
}
