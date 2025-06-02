#version 330 core

in vec3 velocity;
in float velocityMagnitude;

uniform float uMaxVelocity;

out vec4 FragColor;

vec3 velocityToColor(float magnitude, float maxMagnitude)
{
    float normalized = clamp(magnitude / maxMagnitude, 0.0, 1.0);
    
    // Enhanced fluid color mapping for more natural air/water movement visualization
    // Low velocity = Transparent/light blue (still air/water)
    // Medium velocity = Deeper blue/cyan (flowing air/water) 
    // High velocity = Blue-white/cyan-white (fast moving air/water)
    // Maximum velocity = Pure white with slight blue tint (turbulent flow)
    
    if (normalized < 0.15) {
        // Very still air - almost transparent light blue
        float t = normalized / 0.15;
        return mix(vec3(0.9, 0.95, 1.0), vec3(0.7, 0.85, 1.0), t);
    } else if (normalized < 0.4) {
        // Gentle movement - light blue to medium blue
        float t = (normalized - 0.15) / 0.25;
        return mix(vec3(0.7, 0.85, 1.0), vec3(0.4, 0.7, 1.0), t);
    } else if (normalized < 0.65) {
        // Medium flow - blue to cyan with more saturation
        float t = (normalized - 0.4) / 0.25;
        return mix(vec3(0.4, 0.7, 1.0), vec3(0.1, 0.8, 1.0), t);
    } else if (normalized < 0.85) {
        // Fast flow - cyan to bright cyan-white
        float t = (normalized - 0.65) / 0.2;
        return mix(vec3(0.1, 0.8, 1.0), vec3(0.6, 0.95, 1.0), t);
    } else {
        // Very fast/turbulent flow - bright cyan-white to pure white
        float t = (normalized - 0.85) / 0.15;
        return mix(vec3(0.6, 0.95, 1.0), vec3(0.95, 0.98, 1.0), t);
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
