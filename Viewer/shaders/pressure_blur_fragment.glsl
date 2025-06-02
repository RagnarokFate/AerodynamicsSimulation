#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D pressureTexture;
uniform vec2 texelSize;
uniform bool horizontal;
uniform float blurRadius;
uniform float blurStrength;
uniform float minPressure;
uniform float maxPressure;

// Enhanced Gaussian kernel for pressure field smoothing
const float weight[7] = float[] (0.1531531532, 0.1440721649, 0.1221864865, 0.0920454545, 0.0606060606, 0.0357142857, 0.0182186235);

vec3 pressureToColor(float p, float minP, float maxP)
{
    float normalized = clamp((p - minP) / (maxP - minP), 0.0, 1.0);
    
    // Scientific pressure colormap
    if (normalized < 0.2) {
        float t = normalized / 0.2;
        return mix(vec3(0.0, 0.0, 0.8), vec3(0.0, 0.3, 1.0), t);
    } else if (normalized < 0.4) {
        float t = (normalized - 0.2) / 0.2;
        return mix(vec3(0.0, 0.3, 1.0), vec3(0.0, 0.8, 1.0), t);
    } else if (normalized < 0.6) {
        float t = (normalized - 0.4) / 0.2;
        return mix(vec3(0.0, 0.8, 1.0), vec3(0.2, 1.0, 0.2), t);
    } else if (normalized < 0.8) {
        float t = (normalized - 0.6) / 0.2;
        return mix(vec3(0.2, 1.0, 0.2), vec3(1.0, 0.9, 0.0), t);
    } else {
        float t = (normalized - 0.8) / 0.2;
        return mix(vec3(1.0, 0.9, 0.0), vec3(1.0, 0.0, 0.0), t);
    }
}

void main()
{
    // Sample the pressure value at current position
    float centerPressure = texture(pressureTexture, TexCoord).r;
    float blurredPressure = centerPressure * weight[0];
    
    if (horizontal) {
        // Horizontal blur pass
        for(int i = 1; i < 7; ++i) {
            float offset = float(i) * blurRadius;
            blurredPressure += texture(pressureTexture, TexCoord + vec2(texelSize.x * offset, 0.0)).r * weight[i];
            blurredPressure += texture(pressureTexture, TexCoord - vec2(texelSize.x * offset, 0.0)).r * weight[i];
        }
    } else {
        // Vertical blur pass
        for(int i = 1; i < 7; ++i) {
            float offset = float(i) * blurRadius;
            blurredPressure += texture(pressureTexture, TexCoord + vec2(0.0, texelSize.y * offset)).r * weight[i];
            blurredPressure += texture(pressureTexture, TexCoord - vec2(0.0, texelSize.y * offset)).r * weight[i];
        }
    }
    
    // Mix original with blurred pressure based on blur strength
    float finalPressure = mix(centerPressure, blurredPressure, blurStrength);
    
    // Convert to color using pressure colormap
    vec3 color = pressureToColor(finalPressure, minPressure, maxPressure);
    
    FragColor = vec4(color, 1.0);
}
