#version 330 core

in vec3 velocity;
in float velocityMagnitude;

uniform float uMaxVelocity;

out vec4 FragColor;

vec3 velocityToColor(float magnitude, float maxMagnitude)
{
    float normalized = clamp(magnitude / maxMagnitude, 0.0, 1.0);
    
    // Fluid color mapping: Blue tones for air/water movement
    // Low velocity = Deep blue (still air/water)
    // Medium velocity = Cyan/light blue (flowing air/water) 
    // High velocity = White/light cyan (fast moving air/water)
    
    if (normalized < 0.3) {
        // Still to slow movement - deep blue to blue
        float t = normalized / 0.3;
        return mix(vec3(0.0, 0.1, 0.4), vec3(0.0, 0.3, 0.8), t);
    } else if (normalized < 0.6) {
        // Medium flow - blue to cyan
        float t = (normalized - 0.3) / 0.3;
        return mix(vec3(0.0, 0.3, 0.8), vec3(0.0, 0.7, 1.0), t);
    } else if (normalized < 0.8) {
        // Fast flow - cyan to light cyan
        float t = (normalized - 0.6) / 0.2;
        return mix(vec3(0.0, 0.7, 1.0), vec3(0.3, 0.9, 1.0), t);
    } else {
        // Very fast flow - light cyan to white
        float t = (normalized - 0.8) / 0.2;
        return mix(vec3(0.3, 0.9, 1.0), vec3(0.8, 1.0, 1.0), t);
    }
}

void main()
{
    // Use fluid-based coloring for velocity vectors (air/water flow)
    vec3 color = velocityToColor(velocityMagnitude, uMaxVelocity);
    
    // Enhanced visibility with moderate brightness boost for fluid visualization
    float brightnessBoost = 1.3;
    color *= brightnessBoost;
    color = clamp(color, 0.0, 1.0);
    
    FragColor = vec4(color, 0.9); // High alpha for clear velocity visualization
}
