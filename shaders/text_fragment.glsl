#version 330 core
out vec4 FragColor;

in vec2 TexCoords; // Input från vertex shader

// Texturen för font-glyphen (monokrom, använder bara R-kanalen)
uniform sampler2D text;
// Färgen för texten
uniform vec3 textColor;
// // Uniform för att indikera om det är en solid bakgrund eller text
// uniform bool isCursor; // Kommenterad ut tidigare

void main()
{    
    // Hämta alpha-värdet från texturen (röd kanal)
    float alpha = texture(text, TexCoords).r;
    
    // ÅTERSTÄLL: Använd textur-alpha och textColor uniform
    FragColor = vec4(textColor, alpha); 
    
    /* // TEST: Använd alpha från textur, men tvinga RGB till GRÖNT
    FragColor = vec4(0.0, 1.0, 0.0, alpha); 
    */
}
