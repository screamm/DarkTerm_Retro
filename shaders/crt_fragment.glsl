#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

// Texturen som innehåller den renderade terminalbilden (från FBO)
uniform sampler2D screenTexture;

// Uniforms för att styra effekten (från temat)
uniform float time;              // För eventuell flimmer-effekt
uniform float scanlineIntensity; // 0.0 (av) - 1.0 (max)
uniform float curvature;         // 0.0 (av) - ~0.5 (kraftig)
uniform float rgbShift;          // > 0.0 för att aktivera

// Funktion för att böja UV-koordinater för kurvatureffekt
vec2 curveUV(vec2 uv, float curveAmount) {
    // Konvertera till centrerade koordinater (-1 till 1)
    uv = uv * 2.0 - 1.0;

    // Applicera barrel distortion
    // En enkel approximation som fungerar ok
    float curve = curveAmount * 0.1; // Justera styrkan
    vec2 offset = uv.yx * curve;
    uv = uv + uv * offset * offset;

    // Konvertera tillbaka till texturkoordinater (0 till 1)
    uv = uv * 0.5 + 0.5;
    return uv;
}

void main()
{
    // 1. Applicera kurvatur (om aktiverad)
    vec2 curvedTexCoords = (curvature > 0.0) ? curveUV(TexCoords, curvature) : TexCoords;

    // 2. Kontrollera om den böjda koordinaten är utanför skärmen
    if (curvedTexCoords.x < 0.0 || curvedTexCoords.x > 1.0 || curvedTexCoords.y < 0.0 || curvedTexCoords.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); // Svart utanför böjen
        return;
    }

    // 3. Applicera RGB shift (om aktiverad)
    vec3 finalColor;
    if (rgbShift > 0.0) {
        float r = texture(screenTexture, vec2(curvedTexCoords.x + rgbShift, curvedTexCoords.y)).r;
        float g = texture(screenTexture, curvedTexCoords).g;
        float b = texture(screenTexture, vec2(curvedTexCoords.x - rgbShift, curvedTexCoords.y)).b;
        finalColor = vec3(r, g, b);
    } else {
        finalColor = texture(screenTexture, curvedTexCoords).rgb;
    }

    // 4. Applicera scanlines (om aktiverad)
    if (scanlineIntensity > 0.0) {
        // Beräkna scanline-effekt baserat på skärmens y-koordinat
        // Öka frekvensen (t.ex. * 600.0) för tunnare linjer
        float scanLine = sin(curvedTexCoords.y * 600.0) * 0.5 + 0.5;
        // Minska intensiteten för en subtil effekt
        float intensity = pow(scanLine, 2.0) * scanlineIntensity * 0.3 + (1.0 - scanlineIntensity * 0.3);
        finalColor *= intensity;
        // Alternativ, enklare scanline:
        // float scan = mod(gl_FragCoord.y, 2.0) * 0.5 + 0.5;
        // finalColor *= mix(1.0, scan, scanlineIntensity);
    }

    // 5. (Valfritt) Lägg till enkel vinjettering
    float vignetteIntensity = 0.5 + curvature * 2.0; // Mer vinjettering med mer kurvatur
    float vignette = 1.0 - length((curvedTexCoords - 0.5) * 2.0) * vignetteIntensity * 0.2;
    finalColor *= vignette;

    // 6. (Valfritt) Lägg till subtilt flimmer
    // float flicker = 1.0 + (sin(time * 15.0) * 0.01);
    // finalColor *= flicker;

    FragColor = vec4(finalColor, 1.0);
}
