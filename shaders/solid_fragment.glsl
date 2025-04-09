#version 330 core
out vec4 FragColor;

// Färgen för rektangeln
uniform vec3 solidColor;

void main()
{
    // Använd full opacitet (1.0 alpha)
    FragColor = vec4(solidColor, 1.0);
}
