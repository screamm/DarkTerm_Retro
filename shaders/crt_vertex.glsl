#version 330 core
layout (location = 0) in vec2 aPos;      // Vertex position (från helskärms-quad)
layout (location = 1) in vec2 aTexCoords; // Texturkoordinater (från helskärms-quad)

out vec2 TexCoords;

void main()
{
    // Positionen är redan i Normalized Device Coordinates (-1 till 1)
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    // Skicka texturkoordinaterna vidare till fragment shadern
    TexCoords = aTexCoords;
}
