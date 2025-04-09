cmake_minimum_required(VERSION 3.15)
project(DarkTerm LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # Lägg exekverbar fil i build-mappen

# --- Hitta Beroenden (via Homebrew/system) ---
find_package(glfw3 REQUIRED)
find_package(Freetype REQUIRED)
find_package(OpenGL REQUIRED)
# find_package(JsonCpp REQUIRED) # Borttaget, vi länkar manuellt

# Manuellt hitta JsonCpp (anpassa sökvägar vid behov)
find_library(JSONCPP_LIBRARY NAMES jsoncpp PATHS /opt/homebrew/lib NO_DEFAULT_PATH)
find_path(JSONCPP_INCLUDE_DIR NAMES json/json.h PATHS /opt/homebrew/include NO_DEFAULT_PATH)

if(NOT JSONCPP_LIBRARY OR NOT JSONCPP_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find JsonCpp library or include directory. Searched in /opt/homebrew.")
endif()

message(STATUS "Found JsonCpp Library: ${JSONCPP_LIBRARY}")
message(STATUS "Found JsonCpp Include Dir: ${JSONCPP_INCLUDE_DIR}")

# --- Hantera GLAD (lokalt i projektet) ---
# Antag att glad.c finns i src/ och glad/glad.h finns i include/
add_library(glad_lib src/glad.c)
target_include_directories(glad_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Definiera Exekverbar Fil ---
add_executable(DarkTerm
    src/main.cpp
    src/ThemeManager.cpp
    # Lägg till fler .cpp-filer här om du skapar dem
)

# --- Inkludera Headers ---
target_include_directories(DarkTerm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include # För glad/glad.h
    ${JSONCPP_INCLUDE_DIR} # För json/json.h
    # Lägg till andra include-mappar här om nödvändigt
)

# --- Länka Bibliotek ---
target_link_libraries(DarkTerm PRIVATE
    glad_lib          # Vår lokala GLAD-kompilering
    glfw              # Från find_package
    Freetype::Freetype # Från find_package
    OpenGL::GL        # Från find_package
    ${JSONCPP_LIBRARY} # Manuellt hittat bibliotek
)

# --- Plattformsspecifika länkar ---
if(APPLE)
    # På macOS behövs ofta dessa ramverk för GLFW och OpenGL
    target_link_libraries(DarkTerm PRIVATE "-framework Cocoa -framework IOKit -framework CoreVideo -framework OpenGL")
endif()

# --- Kopiera Resurser till Byggmappen (Valfritt men rekommenderat) ---
# Detta gör att programmet hittar filerna när det körs från build-mappen
# Justera sökvägarna om din struktur skiljer sig
file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*.glsl")
file(GLOB FONT_FILES "${CMAKE_SOURCE_DIR}/fonts/*.*tf") # Hitta .ttf eller .otf

add_custom_command(TARGET DarkTerm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:DarkTerm>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/fonts" "$<TARGET_FILE_DIR:DarkTerm>/fonts"
    # COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/themes" "$<TARGET_FILE_DIR:DarkTerm>/themes" # Avkommentera om du har externa teman
    COMMENT "Copying resources to build directory"
)

# För VS Code IntelliSense (hjälper C/C++ extensionen)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
