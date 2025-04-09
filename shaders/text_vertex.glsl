#version 330 core
layout (location = 0) in vec2 aPos; // Förvänta oss direkt 2D NDC-position
layout (location = 1) in vec2 aTexCoords; // Texturkoordinater (u,v)

out vec2 TexCoords; // Skicka vidare till fragment shader

// uniform mat4 projection; // BORTTAGEN

void main()
{
    // Skicka positionen direkt vidare, anta att den redan är i NDC
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoords = aTexCoords; // Skicka texturkoordinaterna vidare
}
