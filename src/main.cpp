#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <memory> // För std::unique_ptr
#include <fstream> // För filhantering (läsa shaders)
#include <sstream> // För att läsa filinnehåll till string

// GLAD måste inkluderas före GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// FreeType för font-rendering
#include <ft2build.h>
#include FT_FREETYPE_H

#include "ThemeManager.h"

// Grundläggande struktur för terminalen
struct RetroTerminal {
    // Fönsteregenskaper
    int width = 800;
    int height = 600;
    const char* title = "RetroTerm - 8-bit Terminal";
    GLFWwindow* window = nullptr;

    // Terminalegenskaper
    int cols = 80;
    int rows = 25;
    int cellWidth = 0; // Beräknas från font
    int cellHeight = 16; // Önskad höjd
    std::vector<std::vector<char>> buffer; // Teckenbuffert
    std::vector<std::vector<int>> colorBuffer; // Färgindex per tecken
    std::vector<std::vector<int>> bgBuffer; // Bakgrundsfärgindex per tecken (för framtida bruk)

    // Cursor-tillstånd
    int cursorX = 0;
    int cursorY = 0;
    bool cursorVisible = true;
    double lastCursorBlinkTime = 0.0;
    double cursorBlinkInterval = 0.5; // Sekunder

    // Temahanterare
    ThemeManager themeManager;

    // Font-rendering
    FT_Library ft_library = nullptr;
    FT_Face ft_face = nullptr;

    struct Character {
        GLuint textureID = 0;
        int width = 0;
        int height = 0;
        int bearingX = 0;
        int bearingY = 0;
        unsigned int advance = 0;
    };
    std::map<char, Character> characters;
    GLuint font_vao = 0, font_vbo = 0;
    GLuint text_shader_program = 0;
    
    // Solid shader för markören
    GLuint solid_shader_program = 0;

    // CRT Shader (valfritt)
    GLuint crt_shader_program = 0;
    GLuint crt_fbo = 0, crt_texture = 0, crt_rbo = 0; // Framebuffer för CRT-effekt
    GLuint crt_vao = 0, crt_vbo = 0;
    bool use_crt_effect = false; // Inaktivera för felsökning

     ~RetroTerminal() {
        // Städa upp FreeType
        if (ft_face) FT_Done_Face(ft_face);
        if (ft_library) FT_Done_FreeType(ft_library);
        // OpenGL-objekt städas upp separat
    }
};

// ----- Funktionsdeklarationer -----
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void char_callback(GLFWwindow* window, unsigned int codepoint);
bool initGLFW(RetroTerminal& term);
bool initGLAD(); // Flyttad från initGLFW
bool initFreeType(RetroTerminal& term);
bool loadFont(RetroTerminal& term, const char* fontPath, int pixelHeight);
bool createTextShaderProgram(RetroTerminal& term);
bool createSolidShaderProgram(RetroTerminal& term);
bool createCRTShaderProgram(RetroTerminal& term);
bool setupFontRendering(RetroTerminal& term);
bool setupCRTRendering(RetroTerminal& term);
void initTerminalBuffer(RetroTerminal& term);
void renderTerminal(RetroTerminal& term, double currentTime);
void cleanup(RetroTerminal& term);
void putChar(RetroTerminal& term, char c, int x, int y, int fgColor, int bgColor);
void scrollBuffer(RetroTerminal& term);
void handleInput(RetroTerminal& term, char c);

// ----- Huvudfunktion ----- 
int main() {
    RetroTerminal term;

    // 1. Initiera GLFW och skapa fönster
    if (!initGLFW(term)) {
        return -1;
    }

    // 2. Initiera GLAD (ladda OpenGL-funktioner)
    if (!initGLAD()) {
        glfwTerminate();
        return -1;
    }

    // 3. Sätt callbacks (efter GLAD-init)
    glfwSetWindowUserPointer(term.window, &term); // Koppla term-instans till fönstret
    glfwSetFramebufferSizeCallback(term.window, framebuffer_size_callback);
    // Kommentera ut key/char callbacks för att minimera störningar
    // glfwSetKeyCallback(term.window, key_callback);
    // glfwSetCharCallback(term.window, char_callback);
    // ÅTERAKTIVERA CALLBACKS
    glfwSetKeyCallback(term.window, key_callback);
    glfwSetCharCallback(term.window, char_callback);

    // /* // KOMMENTERA UT ALL ANNAN INIT // Behåll kommentaren här
    // 4. Initiera FreeType
    if (!initFreeType(term)) {
        // cleanup(term); // Håll cleanup kommenterad
        glfwTerminate(); // Enkel cleanup om det misslyckas
        return -1;
    }

    // 5. Ladda en font (byt sökväg!)
    // VIKTIGT: Byt "fonts/din_retro_font.ttf" till den faktiska sökvägen
    // till din nedladdade fontfil i fonts-mappen.
    if (!loadFont(term, "fonts/Perfect DOS VGA 437.ttf", term.cellHeight)) { // Exempel: Perfect DOS VGA
         std::cerr << "Kunde inte ladda font. Kontrollera sökvägen i main.cpp!" << std::endl;
         // cleanup(term); // Håll cleanup kommenterad
         glfwTerminate(); // Enkel cleanup om det misslyckas
         return -1;
    }
    // Flytta ner denna kommentar - Aktiverar shader och font setup
    // 6. Skapa och kompilera shaders
    if (!createTextShaderProgram(term)) {
        // cleanup(term); // Håll kommenterad
        glfwTerminate();
        return -1;
    }
    
    // Skapa solid shader program för cursor
    if (!createSolidShaderProgram(term)) {
        // cleanup(term);
        glfwTerminate();
        return -1;
    }
    
    if (term.use_crt_effect && !createCRTShaderProgram(term)) {
        // cleanup(term);
        glfwTerminate();
        return -1; // Fortsätt inte om CRT-shadern misslyckas och den är påslagen
    }

    // 7. Sätt upp OpenGL för rendering (VAO/VBOs)
    if (!setupFontRendering(term)) {
        // cleanup(term);
        glfwTerminate();
        return -1;
    }
    if (term.use_crt_effect && !setupCRTRendering(term)) {
        // cleanup(term);
        glfwTerminate();
        return -1;
    }

    // 8. Initiera terminalens textbuffert
    initTerminalBuffer(term);
    // */

    // Sätt initial OpenGL-viewport & state (flyttat hit från innanför kommentaren)
    glfwGetFramebufferSize(term.window, &term.width, &term.height); // Hämta aktuell storlek
    glViewport(0, 0, term.width, term.height);
    glEnable(GL_BLEND); // Även om vi inte använder alpha nu, skadar det inte
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // ----- MINIMAL RENDERING SETUP FÖR FELSÖKNING ----- 
    // TA BORT HELA DETTA BLOCK
    /*
    const char *minimalVertexShaderSource = R"glsl(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
        }
    )glsl";
    const char *minimalFragmentShaderSource = R"glsl(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 1.0, 1.0, 1.0); // Rita vitt
        }
    )glsl";

    GLuint minimalVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(minimalVertexShader, 1, &minimalVertexShaderSource, NULL);
    glCompileShader(minimalVertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(minimalVertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(minimalVertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::MINIMAL_VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    GLuint minimalFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(minimalFragmentShader, 1, &minimalFragmentShaderSource, NULL);
    glCompileShader(minimalFragmentShader);
    glGetShaderiv(minimalFragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(minimalFragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::MINIMAL_FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    GLuint minimalShaderProgram = glCreateProgram();
    glAttachShader(minimalShaderProgram, minimalVertexShader);
    glAttachShader(minimalShaderProgram, minimalFragmentShader);
    glLinkProgram(minimalShaderProgram);
    glGetProgramiv(minimalShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(minimalShaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::MINIMAL_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(minimalVertexShader);
    glDeleteShader(minimalFragmentShader);

    float minimalVertices[] = {
        // Triangel 1
         0.5f,  0.5f, // Top Right
         0.5f, -0.5f, // Bottom Right
        -0.5f,  0.5f, // Top Left 
        // Triangel 2
         0.5f, -0.5f, // Bottom Right
        -0.5f, -0.5f, // Bottom Left
        -0.5f,  0.5f  // Top Left
    }; 
    GLuint minimalVAO, minimalVBO;
    glGenVertexArrays(1, &minimalVAO);
    glGenBuffers(1, &minimalVBO);
    glBindVertexArray(minimalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, minimalVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(minimalVertices), minimalVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
    
    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error after minimal setup: " << err << std::endl;
    }
    */
    // ----- SLUT MINIMAL RENDERING SETUP -----


    // ------ Huvudloop ------
    while (!glfwWindowShouldClose(term.window)) {
        // Hämta aktuell tid för animationer (cursor blink, CRT effect)
        double currentTime = glfwGetTime();

        // Hantera input
        glfwPollEvents(); // Kollar efter fönsterhändelser, anropar callbacks

        // Uppdatera terminalens tillstånd (t.ex. cursor blink)
        // ÅTERAKTIVERA CURSOR BLINK
        /* // Kommentera ut blink-logiken */ // TA BORT KOMMENTAR
       if (currentTime - term.lastCursorBlinkTime >= term.cursorBlinkInterval) {
           term.cursorVisible = !term.cursorVisible;
           term.lastCursorBlinkTime = currentTime;
       }
       // */ // TA BORT KOMMENTAR
       // term.cursorVisible = true; // Tvinga markören att alltid vara synlig för test // TA BORT DENNA RAD

        // ----- MINIMAL RENDERING ----- 
        // TA BORT DETTA BLOCK
        /*
        glViewport(0, 0, term.width, term.height); // Kontrollera fel
        err = glGetError(); if(err != GL_NO_ERROR) std::cerr << "OpenGL error before clear: " << err << std::endl;
        glClearColor(0.0f, 0.0f, 0.2f, 1.0f); // Mörkblå bakgrund
        glClear(GL_COLOR_BUFFER_BIT);
        err = glGetError(); if(err != GL_NO_ERROR) std::cerr << "OpenGL error after clear: " << err << std::endl;
        glUseProgram(minimalShaderProgram);
        err = glGetError(); if(err != GL_NO_ERROR) std::cerr << "OpenGL error after useProgram: " << err << std::endl;
        glBindVertexArray(minimalVAO);
        err = glGetError(); if(err != GL_NO_ERROR) std::cerr << "OpenGL error after bindVAO: " << err << std::endl;
        glDrawArrays(GL_TRIANGLES, 0, 6); // Rita 6 hörn (två trianglar)
        err = glGetError(); if(err != GL_NO_ERROR) std::cerr << "OpenGL error after drawArrays: " << err << std::endl;
        glBindVertexArray(0); // Unbind
        */
        // ----- SLUT MINIMAL RENDERING -----


        // EXTREM FÖRENKLING - Rita bara en röd rektangel för att se om något syns alls
        // (Kommenterar ut den tidigare rektangel-koden)
        // TA BORT DETTA BLOCK HELT
        /*
        ... (den gamla röd-rektangel-koden) ...
        */
        
        // Rendera terminalen (vanlig rendering) - KOMMENTERAD UT
        // renderTerminal(term, currentTime);
        // ÅTERAKTIVERA RENDERTERMINAL
        renderTerminal(term, currentTime);

        // Byt buffertar (visa det som ritats)
        glfwSwapBuffers(term.window);
        // Ta bort felkontroll härifrån?
        // err = glGetError(); if(err != GL_NO_ERROR) std::cerr << "OpenGL error after swapBuffers: " << err << std::endl;
    }

    // ------ Städa upp minimalt ------
    // TA BORT DETTA BLOCK
    /*
    // Glöm inte att städa upp de nya minimala resurserna
    glDeleteVertexArrays(1, &minimalVAO);
    glDeleteBuffers(1, &minimalVBO);
    glDeleteProgram(minimalShaderProgram);
    
    // Kommentera ut den vanliga cleanup
    // cleanup(term); 

    // Minimal GLFW cleanup
    if (term.window) {
        glfwDestroyWindow(term.window);
    }
    glfwTerminate();
    */
    // ÅTERAKTIVERA VANLIG CLEANUP
    cleanup(term);

    return 0;
}

// ----- Funktionsimplementationer -----

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Uppdatera OpenGL viewport när fönstret storleksändras
    glViewport(0, 0, width, height);

    // Uppdatera terminalens storlek (kan behöva mer logik för att
    // anpassa cols/rows eller cellstorlek)
    RetroTerminal* term = (RetroTerminal*)glfwGetWindowUserPointer(window);
    if (term) {
        term->width = width;
        term->height = height;
        // TODO: Potentiellt uppdatera CRT framebuffer-storlek här
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    RetroTerminal* term = (RetroTerminal*)glfwGetWindowUserPointer(window);
    if (!term) return;

    // Hantera endast knapptryckningar (inte repeat som standard, hanteras av char_callback)
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, true);
                break;
            case GLFW_KEY_ENTER:
                handleInput(*term, '\n'); // Skicka newline
                break;
            case GLFW_KEY_BACKSPACE:
                 handleInput(*term, '\b'); // Skicka backspace
                 break;
            case GLFW_KEY_TAB:
                 handleInput(*term, '\t'); // Skicka tab
                 break;
             // TODO: Hantera piltangenter, Home, End, PgUp, PgDown etc.
             // för att flytta cursorn eller skicka escape-sekvenser
             case GLFW_KEY_LEFT:
                if (term->cursorX > 0) term->cursorX--;
                term->cursorVisible = true; term->lastCursorBlinkTime = glfwGetTime();
                break;
             case GLFW_KEY_RIGHT:
                 if (term->cursorX < term->cols - 1) term->cursorX++;
                 term->cursorVisible = true; term->lastCursorBlinkTime = glfwGetTime();
                 break;
             case GLFW_KEY_UP:
                 if (term->cursorY > 0) term->cursorY--;
                 term->cursorVisible = true; term->lastCursorBlinkTime = glfwGetTime();
                 break;
             case GLFW_KEY_DOWN:
                 if (term->cursorY < term->rows - 1) term->cursorY++;
                 term->cursorVisible = true; term->lastCursorBlinkTime = glfwGetTime();
                 break;
            // Exempel: Byt tema med F1
            case GLFW_KEY_F1:
            {
                auto names = term->themeManager.getThemeNames();
                if (!names.empty()) {
                    static size_t current_theme_index = 0;
                    current_theme_index = (current_theme_index + 1) % names.size();
                    term->themeManager.setTheme(names[current_theme_index]);
                    std::cout << "Changed theme to: " << names[current_theme_index] << std::endl;
                }
                break;
            }
        }
    }
}

void char_callback(GLFWwindow* window, unsigned int codepoint) {
    RetroTerminal* term = (RetroTerminal*)glfwGetWindowUserPointer(window);
    if (!term) return;

    // Konvertera Unicode codepoint till char (för enkel ASCII just nu)
    if (codepoint < 128) { // TODO: Hantera UTF-8 korrekt
        handleInput(*term, static_cast<char>(codepoint));
    }
}

bool initGLFW(RetroTerminal& term) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // Tillåt storleksändring

    // Skapa fönstret med initial storlek (kommer justeras av loadFont)
    term.window = glfwCreateWindow(term.width, term.height, term.title, NULL, NULL);
    if (!term.window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(term.window);
    glfwSwapInterval(1); // Aktivera vsync

    return true;
}

bool initGLAD() {
    // Initiera GLAD för att ladda OpenGL-funktionspekare
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    return true;
}

bool initFreeType(RetroTerminal& term) {
    if (FT_Init_FreeType(&term.ft_library)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return false;
    }
    return true;
}

bool loadFont(RetroTerminal& term, const char* fontPath, int pixelHeight) {
    if (!term.ft_library) return false;

    // Ladda font face
    if (FT_New_Face(term.ft_library, fontPath, 0, &term.ft_face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font: " << fontPath << std::endl;
        return false;
    }

    // Sätt storlek (bredd 0 låter FreeType bestämma baserat på höjd)
    FT_Set_Pixel_Sizes(term.ft_face, 0, pixelHeight);
    term.cellHeight = pixelHeight; // Bekräfta cellhöjd

    // Ta reda på cellbredd (använd bredden av 'W' eller medelbredd)
    if (FT_Load_Char(term.ft_face, 'W', FT_LOAD_RENDER)) {
         std::cerr << "Warning: Failed to load glyph 'W' to determine width." << std::endl;
         term.cellWidth = pixelHeight / 2; // Gör en gissning
    } else {
         // Advance är hur mycket cursorn ska flyttas efter tecknet
         term.cellWidth = term.ft_face->glyph->advance.x >> 6; // Konvertera 1/64 pixlar till pixlar
         if (term.cellWidth <= 0) term.cellWidth = pixelHeight / 2; // Fallback
    }
     std::cout << "Font loaded: " << fontPath << " Cell size: " << term.cellWidth << "x" << term.cellHeight << std::endl;


    // Justera fönsterstorleken baserat på fontens cellstorlek och terminalens dimensioner
    term.width = term.cols * term.cellWidth;
    term.height = term.rows * term.cellHeight;
    glfwSetWindowSize(term.window, term.width, term.height);

    // Ladda teckenglyphs för ASCII 0-127
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Inaktivera byte-alignment restriction

    for (unsigned char c = 0; c < 128; c++) {
        // Ladda teckenglyph
        if (FT_Load_Char(term.ft_face, c, FT_LOAD_RENDER)) {
            std::cerr << "Warning::FREETYPE: Failed to load Glyph: " << c << std::endl;
            continue;
        }
        // Generera textur
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED, // Använd röd kanal för monokrom textur
            term.ft_face->glyph->bitmap.width,
            term.ft_face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            term.ft_face->glyph->bitmap.buffer
        );
        // Sätt texturinställningar
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Använd GL_NEAREST för pixel-perfekt retro-look
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Lagra tecken för senare användning
        RetroTerminal::Character character = {
            texture,
            (int)term.ft_face->glyph->bitmap.width,
            (int)term.ft_face->glyph->bitmap.rows,
            term.ft_face->glyph->bitmap_left,
            term.ft_face->glyph->bitmap_top,
            (unsigned int)(term.ft_face->glyph->advance.x >> 6) // advance i pixlar
        };
        term.characters.insert(std::pair<char, RetroTerminal::Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // FreeType-resurser (face) kan rensas nu om vi inte behöver ladda om tecken dynamiskt
    // FT_Done_Face(term.ft_face);
    // term.ft_face = nullptr; // Men vi behåller det för ev. framtida bruk

    return true;
}

// Funktion för att läsa shader-kod från fil
std::string readShaderFile(const std::string& filePath) {
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open()) {
        std::cerr << "ERROR::SHADER: Failed to open file: " << filePath << std::endl;
        return "";
    }
    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();
    shaderFile.close();
    return shaderStream.str();
}

// Funktion för att kompilera shader och länka program
GLuint compileAndLinkShaders(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCodeStr = readShaderFile(vertexPath);
    std::string fragmentCodeStr = readShaderFile(fragmentPath);
    if (vertexCodeStr.empty() || fragmentCodeStr.empty()) {
        return 0;
    }
    const char* vShaderCode = vertexCodeStr.c_str();
    const char* fShaderCode = fragmentCodeStr.c_str();

    GLuint vertex, fragment;
    int success;
    char infoLog[512];

    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << vertexPath << "\n" << infoLog << std::endl;
        glDeleteShader(vertex);
        return 0;
    }

    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << fragmentPath << "\n" << infoLog << std::endl;
        glDeleteShader(vertex); // Städa upp vertex shadern också
        glDeleteShader(fragment);
        return 0;
    }

    // Shader Program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteProgram(shaderProgram); // Städa upp programmet
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    // Radera shaders då de är länkade till programmet och inte längre behövs
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return shaderProgram;
}

bool createTextShaderProgram(RetroTerminal& term) {
    term.text_shader_program = compileAndLinkShaders("shaders/text_vertex.glsl", "shaders/text_fragment.glsl");
    return term.text_shader_program != 0;
}

bool createSolidShaderProgram(RetroTerminal& term) {
    std::cout << "Försöker ladda solid shader filer..." << std::endl;
    
    std::string vertexCode = readShaderFile("shaders/solid_vertex.glsl");
    std::string fragmentCode = readShaderFile("shaders/solid_fragment.glsl");
    
    std::cout << "solid_vertex.glsl innehåll (" << vertexCode.length() << " bytes):" << std::endl;
    std::cout << vertexCode << std::endl;
    
    std::cout << "solid_fragment.glsl innehåll (" << fragmentCode.length() << " bytes):" << std::endl;
    std::cout << fragmentCode << std::endl;
    
    term.solid_shader_program = compileAndLinkShaders("shaders/solid_vertex.glsl", "shaders/solid_fragment.glsl");
    std::cout << "solid_shader_program: " << term.solid_shader_program << std::endl;
    return term.solid_shader_program != 0;
}

bool createCRTShaderProgram(RetroTerminal& term) {
     term.crt_shader_program = compileAndLinkShaders("shaders/crt_vertex.glsl", "shaders/crt_fragment.glsl");
    return term.crt_shader_program != 0;
}

bool setupFontRendering(RetroTerminal& term) {
    if (term.text_shader_program == 0) return false;

    // Tala om för OpenGL att text-shaderns sampler "text" ska använda textur-enhet 0
    glUseProgram(term.text_shader_program);
    glUniform1i(glGetUniformLocation(term.text_shader_program, "text"), 0);

    glGenVertexArrays(1, &term.font_vao);
    glGenBuffers(1, &term.font_vbo);
    glBindVertexArray(term.font_vao);
    glBindBuffer(GL_ARRAY_BUFFER, term.font_vbo);
    // ÅTERSTÄLL: VBO för 6 hörn med 4 floats (NDC pos.x/y, Tex u/v)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    // Position attribute (location 0 i vertex shader - 2 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // Texture coord attribute (location 1 i vertex shader - 2 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

bool setupCRTRendering(RetroTerminal& term) {
    if (term.crt_shader_program == 0) return false;

    // 1. Skapa Framebuffer Object (FBO)
    glGenFramebuffers(1, &term.crt_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, term.crt_fbo);

    // 2. Skapa textur att rendera till
    glGenTextures(1, &term.crt_texture);
    glBindTexture(GL_TEXTURE_2D, term.crt_texture);
    // Skapa tom textur med fönstrets storlek
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, term.width, term.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind
    // Fäst texturen till FBO:n
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, term.crt_texture, 0);

    // 3. Skapa Renderbuffer Object (RBO) för depth/stencil (om nödvändigt)
    // Vi behöver oftast inte depth/stencil för 2D-terminal, men det är god praxis
    glGenRenderbuffers(1, &term.crt_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, term.crt_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, term.width, term.height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0); // Unbind
    // Fäst RBO:n till FBO:n
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, term.crt_rbo);

    // 4. Kontrollera om FBO är komplett
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // TODO: Städa upp FBO, textur, RBO här vid fel
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Återställ till default framebuffer

    // 5. Skapa VAO/VBO för att rita en helskärms-quad som texturen ska mappas på
    float quadVertices[] = { // Hörnpositioner   Texturkoordinater
        -1.0f,  1.0f,       0.0f, 1.0f,
        -1.0f, -1.0f,       0.0f, 0.0f,
         1.0f, -1.0f,       1.0f, 0.0f,

        -1.0f,  1.0f,       0.0f, 1.0f,
         1.0f, -1.0f,       1.0f, 0.0f,
         1.0f,  1.0f,       1.0f, 1.0f
    };

    glGenVertexArrays(1, &term.crt_vao);
    glGenBuffers(1, &term.crt_vbo);
    glBindVertexArray(term.crt_vao);
    glBindBuffer(GL_ARRAY_BUFFER, term.crt_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    // Position attribute (location 0 i crt_vertex.glsl)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // Texture coord attribute (location 1 i crt_vertex.glsl)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    return true;
}


void initTerminalBuffer(RetroTerminal& term) {
    const auto& currentTheme = term.themeManager.getCurrentTheme();
    term.buffer.assign(term.rows, std::vector<char>(term.cols, ' ')); // Fyll med mellanslag
    term.colorBuffer.assign(term.rows, std::vector<int>(term.cols, currentTheme.fgColor));
    term.bgBuffer.assign(term.rows, std::vector<int>(term.cols, currentTheme.bgColor));
    term.cursorX = 0;
    term.cursorY = 0;
}

// Funktion för att sätta ett tecken i bufferten
void putChar(RetroTerminal& term, char c, int x, int y, int fgColor, int bgColor) {
    if (x >= 0 && x < term.cols && y >= 0 && y < term.rows) {
        term.buffer[y][x] = c;
        term.colorBuffer[y][x] = fgColor;
        term.bgBuffer[y][x] = bgColor;
    }
}

// Funktion för att scrolla bufferten en rad uppåt
void scrollBuffer(RetroTerminal& term) {
    const auto& currentTheme = term.themeManager.getCurrentTheme();
    // Flytta alla rader ett steg upp
    for (int y = 0; y < term.rows - 1; ++y) {
        term.buffer[y] = std::move(term.buffer[y + 1]);
        term.colorBuffer[y] = std::move(term.colorBuffer[y + 1]);
        term.bgBuffer[y] = std::move(term.bgBuffer[y + 1]);
    }
    // Rensa den sista raden
    term.buffer[term.rows - 1].assign(term.cols, ' ');
    term.colorBuffer[term.rows - 1].assign(term.cols, currentTheme.fgColor);
    term.bgBuffer[term.rows - 1].assign(term.cols, currentTheme.bgColor);
}

// Hantera enkel textinput
void handleInput(RetroTerminal& term, char c) {
    const auto& currentTheme = term.themeManager.getCurrentTheme();

    switch (c) {
        case '\n': // Enter
            term.cursorX = 0;
            term.cursorY++;
            break;
        case '\b': // Backspace
            if (term.cursorX > 0) {
                term.cursorX--;
                putChar(term, ' ', term.cursorX, term.cursorY, currentTheme.fgColor, currentTheme.bgColor);
            } else if (term.cursorY > 0) {
                // Flytta upp till slutet av föregående rad
                term.cursorY--;
                term.cursorX = term.cols - 1;
                 // Optional: radera tecknet där (om det inte är mellanslag redan)
                // putChar(term, ' ', term.cursorX, term.cursorY, currentTheme.fgColor, currentTheme.bgColor);
            }
            break;
         case '\t': // Tab (flytta till nästa tabstopp, typiskt var 8:e kolumn)
             {
                 int nextTabStop = (term.cursorX / 8 + 1) * 8;
                 term.cursorX = std::min(term.cols - 1, nextTabStop);
             }
            break;
        default:
            // Skriv ut normalt tecken
            if (c >= 32 && c < 127) { // Skrivbara ASCII
                putChar(term, c, term.cursorX, term.cursorY, currentTheme.fgColor, currentTheme.bgColor);
                term.cursorX++;
            }
            break;
    }

    // Hantera radbrytning
    if (term.cursorX >= term.cols) {
        term.cursorX = 0;
        term.cursorY++;
    }

    // Hantera scrollning
    if (term.cursorY >= term.rows) {
        scrollBuffer(term);
        term.cursorY = term.rows - 1; // Stanna på sista raden
    }

    // Gör cursorn synlig direkt efter input
    term.cursorVisible = true;
    term.lastCursorBlinkTime = glfwGetTime();
}

void renderTerminal(RetroTerminal& term, double currentTime) {
    const auto& currentTheme = term.themeManager.getCurrentTheme();
    const auto& palette = currentTheme.palette;
    
    std::cout << "DEBUG: renderTerminal called. Window size: " << term.width << "x" << term.height << std::endl;

    // ------ Steg 1: Rendera terminalen till FBO (om CRT-effekt är på) ------
    if (term.use_crt_effect) {
        glBindFramebuffer(GL_FRAMEBUFFER, term.crt_fbo);
        // Set viewport to match FBO texture size
        glViewport(0, 0, term.width, term.height);
    }

    // Rensa skärmen/FBO:n med bakgrundsfärgen från temat
    float bgR = 0.0f, bgG = 0.0f, bgB = 0.0f;
    try {
         const auto& bgColor = palette.at(currentTheme.bgColor);
         bgR = bgColor.r; bgG = bgColor.g; bgB = bgColor.b;
         glClearColor(bgR, bgG, bgB, 1.0f);
    } catch(const std::out_of_range& oor) {
         glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Fallback till svart
    }
    std::cout << "DEBUG: Clearing color to: " << bgR << ", " << bgG << ", " << bgB << std::endl;
    glClear(GL_COLOR_BUFFER_BIT);

    // DEBUG: Verifiera nyckelfärger från temat
    try {
        const auto& defaultBg = palette.at(currentTheme.bgColor);
        const auto& cursorFg = palette.at(15); // Förväntad vit
        const auto& cursorBg = palette.at(0); // Förväntad svart (om den finns)
        std::cout << "DEBUG THEME COLORS: DefaultBG(idx " << currentTheme.bgColor << ")=" 
                  << defaultBg.r << "," << defaultBg.g << "," << defaultBg.b 
                  << " | CursorFG(idx 15)=" << cursorFg.r << "," << cursorFg.g << "," << cursorFg.b
                  << " | CursorBG(idx 0)=" << cursorBg.r << "," << cursorBg.g << "," << cursorBg.b 
                  << std::endl;
    } catch(const std::out_of_range& oor) {
        std::cerr << "DEBUG THEME COLORS: Error looking up theme colors 0, 15, or defaultBG." << std::endl;
    }

    // Aktivera text-shadern
    glUseProgram(term.text_shader_program);

    // Aktivera textur-enhet 0 (viktigt!) - Behålls ifall vi återaktiverar textur
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(term.font_vao); // Bind VAO för teckenrendering

    // ----- ÅTERAKTIVERA RITLOOP MED NDC -----
    float ndc_cell_w = 2.0f / static_cast<float>(term.cols);
    float ndc_cell_h = 2.0f / static_cast<float>(term.rows);

    for (int y = 0; y < term.rows; y++) {
        for (int x = 0; x < term.cols; x++) {
            char c = term.buffer[y][x];
            if (c == ' ' || c == '\0') continue; // Hoppa över tomma celler

            int fgColorIndex = term.colorBuffer[y][x];

            // Ändra tecken/färg om det är markörens position och den är synlig
            if (x == term.cursorX && y == term.cursorY && term.cursorVisible) {
                c = (char)219; // Fyllt block-tecken
                fgColorIndex = 15; // Ljust vit
            }

            // DEBUG: Specifik loggning för markörens cell INUTI loopen
            bool isCursorCell = (x == term.cursorX && y == term.cursorY);
            if (isCursorCell) {
                std::cout << "DEBUG LOOP (CURSOR CELL): Processing (" << x << "," << y << ") - Char: '" 
                          << c << "' (dec " << (int)c << ") - FgIdx: " << fgColorIndex << std::endl;
            }

            try {
                const auto& ch = term.characters.at(c);
                const auto& fgColor = palette.at(fgColorIndex);
                
                if (isCursorCell) {
                    std::cout << "DEBUG LOOP (CURSOR CELL): Got char/color. Setting Uniform RGB: " 
                              << fgColor.r << "," << fgColor.g << "," << fgColor.b << std::endl;
                }

                // Sätt färg för tecknet
                glUniform3f(glGetUniformLocation(term.text_shader_program, "textColor"), fgColor.r, fgColor.g, fgColor.b);

                // Beräkna position och storlek direkt i NDC (-1 till 1)
                // Y inverteras: rad 0 är högst upp (nära NDC +1)
                float ndc_center_x = (x + 0.5f) * ndc_cell_w - 1.0f;
                float ndc_center_y = 1.0f - (y + 0.5f) * ndc_cell_h;
                
                // Använd cellens storlek i NDC som bas (kan finjusteras senare)
                float ndc_w = ndc_cell_w;
                float ndc_h = ndc_cell_h;
                
                // Justera eventuellt för tecknets faktiska storlek och bearing relativt cellen?
                // För enkelhetens skull ignorerar vi detta just nu och ritar centrerat i cellen.

                float ndc_left = ndc_center_x - ndc_w / 2.0f;
                float ndc_right = ndc_center_x + ndc_w / 2.0f;
                float ndc_bottom = ndc_center_y - ndc_h / 2.0f;
                float ndc_top = ndc_center_y + ndc_h / 2.0f;

                // Vertices med 4 floats (NDC X, NDC Y, Tex U, Tex V) - Flippad V
                float vertices[6][4] = {
                    { ndc_left,  ndc_bottom,   0.0f, 1.0f },
                    { ndc_right, ndc_bottom,   1.0f, 1.0f },
                    { ndc_right, ndc_top,      1.0f, 0.0f },
                    { ndc_left,  ndc_bottom,   0.0f, 1.0f },
                    { ndc_right, ndc_top,      1.0f, 0.0f },
                    { ndc_left,  ndc_top,      0.0f, 0.0f }
                };

                // Aktivera och bind textur för tecknet
                glBindTexture(GL_TEXTURE_2D, ch.textureID);
                
                // Uppdatera VBO-innehåll
                glBindBuffer(GL_ARRAY_BUFFER, term.font_vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                // Lossa VBO direkt efter uppdatering (god praxis)
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                
                if (isCursorCell) {
                    std::cout << "DEBUG LOOP (CURSOR CELL): Calling glDrawArrays for cursor cell." << std::endl;
                }
                // Rita tecknet (VAO är fortfarande bunden)
                glDrawArrays(GL_TRIANGLES, 0, 6);

            } catch (const std::out_of_range& oor) {
                if (isCursorCell) {
                     std::cerr << "DEBUG LOOP (CURSOR CELL): ERROR finding char '" << c << "' or color index " << fgColorIndex << std::endl;
                } else {
                    // Ignorera om tecken eller färg inte finns
                     std::cerr << "Warning: Character '" << c << "' or color index " << fgColorIndex << " not found." << std::endl;
                }
            }
        }
    }
    // ----- SLUT PÅ RITLOOP -----    

    // Unbind efter all textrendering
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // ------ Steg 2: Rendera FBO till skärmen med CRT-effekt (om på) ------
    if (term.use_crt_effect) {
        // Återgå till default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Återställ viewport till fönstrets storlek
        glViewport(0, 0, term.width, term.height);

        // Rensa default framebuffer (kan vara bra om effekten inte täcker allt)
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Använd CRT-shadern
        glUseProgram(term.crt_shader_program);

        // Skicka uniforms till CRT-shadern
        glUniform1f(glGetUniformLocation(term.crt_shader_program, "time"), (float)currentTime);
        glUniform1f(glGetUniformLocation(term.crt_shader_program, "scanlineIntensity"), currentTheme.scanlineIntensity);
        glUniform1f(glGetUniformLocation(term.crt_shader_program, "curvature"), currentTheme.curvature);
        glUniform1f(glGetUniformLocation(term.crt_shader_program, "rgbShift"), currentTheme.rgbShift);
        glUniform1i(glGetUniformLocation(term.crt_shader_program, "screenTexture"), 0); // Säg åt shadern att använda textur-enhet 0

        // Bind VAO för helskärms-quaden
        glBindVertexArray(term.crt_vao);
        // Bind texturen som vi renderade till i FBO:n
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, term.crt_texture);

        // Rita helskärms-quaden
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Om CRT-effekt inte används, gjordes renderingen direkt till default framebuffer
}


void cleanup(RetroTerminal& term) {
    // Städa upp OpenGL-resurser
    glDeleteVertexArrays(1, &term.font_vao);
    glDeleteBuffers(1, &term.font_vbo);
    glDeleteProgram(term.text_shader_program);
    
    if (term.solid_shader_program != 0) {
        glDeleteProgram(term.solid_shader_program);
    }
    
    if (term.use_crt_effect) {
        glDeleteVertexArrays(1, &term.crt_vao);
        glDeleteBuffers(1, &term.crt_vbo);
        glDeleteFramebuffers(1, &term.crt_fbo);
        glDeleteTextures(1, &term.crt_texture);
        glDeleteRenderbuffers(1, &term.crt_rbo);
        glDeleteProgram(term.crt_shader_program);
    }

    // Städa upp FreeType-tecken texturer
    for (auto const& [key, val] : term.characters) {
        glDeleteTextures(1, &val.textureID);
    }

    // RetroTerminal destruktor hanterar FreeType library/face

    // Städa upp GLFW
    if (term.window) {
        glfwDestroyWindow(term.window);
    }
    glfwTerminate();
}
