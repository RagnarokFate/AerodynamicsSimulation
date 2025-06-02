#version 330 core

in vec3 velocity;
in float velocityMagnitude;

uniform float uMaxVelocity;
uniform float uParticleAlpha;

out vec4 FragColor;

vec3 velocityToColor(float magnitude, float maxMagnitude)
{
    float normalized = clamp(magnitude / maxMagnitude, 0.0, 1.0);
    
    // Enhanced particle coloring for natural fluid flow visualization
    // Uses a more scientific approach with warm-to-cool color mapping
    
    if (normalized < 0.2) {
        // Very low velocity - cool blue (calm regions)
        float t = normalized / 0.2;
        return mix(vec3(0.2, 0.4, 0.8), vec3(0.3, 0.6, 0.9), t);
    } else if (normalized < 0.4) {
        // Low velocity - blue to cyan transition
        float t = (normalized - 0.2) / 0.2;
        return mix(vec3(0.3, 0.6, 0.9), vec3(0.1, 0.7, 0.9), t);
    } else if (normalized < 0.6) {
        // Medium velocity - cyan to green-cyan
        float t = (normalized - 0.4) / 0.2;
        return mix(vec3(0.1, 0.7, 0.9), vec3(0.2, 0.8, 0.6), t);
    } else if (normalized < 0.8) {
        // High velocity - green to yellow (energetic regions)
        float t = (normalized - 0.6) / 0.2;
        return mix(vec3(0.2, 0.8, 0.6), vec3(0.8, 0.9, 0.3), t);
    } else {
        // Very high velocity - yellow to white-orange (turbulent)
        float t = (normalized - 0.8) / 0.2;
        return mix(vec3(0.8, 0.9, 0.3), vec3(1.0, 0.95, 0.8), t);
    }
}
    } else {
        float t = (normalized - 0.66) / 0.34;
        return mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), t);
    }
}

void main()
{
    // Create more natural circular particles with soft volumetric appearance
    vec2 coord = gl_PointCoord - vec2(0.5);
    float distance = length(coord);
    if(distance > 0.5)
        discard;
    
    // Create a volumetric particle with soft falloff (like real fluid particles)
    float alpha = 1.0 - smoothstep(0.2, 0.5, distance);
    alpha = pow(alpha, 1.5); // More dramatic falloff for natural look
    
    // Add some inner glow for more realistic appearance
    float innerGlow = 1.0 - smoothstep(0.0, 0.3, distance);
    
    // Use enhanced velocity-based coloring
    vec3 color = velocityToColor(velocityMagnitude, uMaxVelocity);
    
    // Add subtle brightness variation based on particle center distance
    float centerBrightness = 1.0 + innerGlow * 0.3;
    color *= centerBrightness;
    
    // Enhanced brightness for better visibility against white background
    float brightnessBoost = 2.2;
    color *= brightnessBoost;
    color = clamp(color, 0.0, 1.0);
    
    // Adjust alpha for better visibility and natural blending
    float finalAlpha = uParticleAlpha * alpha * 0.8;
    FragColor = vec4(color, finalAlpha);
}
