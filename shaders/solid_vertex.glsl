#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords> behåller samma layout som text_shader

// Projektionsmatris för att mappa till skärmkoordinater
uniform mat4 projection;

void main()
{
    // OBSERVERA: Vi behåller samma vertex layout som i text-shadern
    // men ignorerar texturkoordinaterna (vertex.zw)
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
}
