#version 330 core

in vec3 velocity;
in float velocityMagnitude;

uniform float uMaxVelocity;
uniform float uParticleAlpha;

out vec4 FragColor;

vec3 velocityToColor(float magnitude, float maxMagnitude)
{
    float normalized = clamp(magnitude / maxMagnitude, 0.0, 1.0);
    
    // Pressure heatmap: Blue (low pressure) to Red (high pressure)
    // This represents pressure distribution rather than velocity
    
    if (normalized < 0.2) {
        // Very low pressure - deep blue to blue
        float t = normalized / 0.2;
        return mix(vec3(0.0, 0.0, 0.6), vec3(0.0, 0.2, 0.8), t);
    } else if (normalized < 0.4) {
        // Low pressure - blue to cyan
        float t = (normalized - 0.2) / 0.2;
        return mix(vec3(0.0, 0.2, 0.8), vec3(0.0, 0.6, 1.0), t);
    } else if (normalized < 0.6) {
        // Medium pressure - cyan to green
        float t = (normalized - 0.4) / 0.2;
        return mix(vec3(0.0, 0.6, 1.0), vec3(0.0, 0.8, 0.2), t);
    } else if (normalized < 0.8) {
        // High pressure - green to yellow/orange
        float t = (normalized - 0.6) / 0.2;
        return mix(vec3(0.0, 0.8, 0.2), vec3(1.0, 0.8, 0.0), t);
    } else {
        // Very high pressure - orange to red
        float t = (normalized - 0.8) / 0.2;
        return mix(vec3(1.0, 0.8, 0.0), vec3(1.0, 0.1, 0.0), t);
    }
}
    } else {
        float t = (normalized - 0.66) / 0.34;
        return mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), t);
    }
}

void main()
{
    // Make particles circular with smooth edges
    vec2 coord = gl_PointCoord - vec2(0.5);
    float distance = length(coord);
    if(distance > 0.5)
        discard;
    
    // Create a soft circular particle with falloff
    float alpha = 1.0 - smoothstep(0.3, 0.5, distance);
    
    // Use pressure-based heatmap coloring for particles
    vec3 color = velocityToColor(velocityMagnitude, uMaxVelocity);
    
    // Enhanced brightness for pressure visualization
    float brightnessBoost = 1.8;
    color *= brightnessBoost;
    color = clamp(color, 0.0, 1.0);
    
    // Increase particle alpha for better visibility
    float finalAlpha = uParticleAlpha * alpha * 0.9;
    FragColor = vec4(color, finalAlpha);
}
