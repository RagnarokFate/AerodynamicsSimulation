#version 330 core

in float pressure;

uniform float uMinPressure;
uniform float uMaxPressure;

out vec4 FragColor;

vec3 pressureToColor(float p, float minP, float maxP)
{
    float normalized = clamp((p - minP) / (maxP - minP), 0.0, 1.0);
    
    // Enhanced pressure heatmap: Blue (low) to Red (high)
    if (normalized < 0.2) {
        // Very low pressure - deep blue to blue
        float t = normalized / 0.2;
        return mix(vec3(0.0, 0.0, 0.8), vec3(0.0, 0.3, 1.0), t);
    } else if (normalized < 0.4) {
        // Low pressure - blue to cyan
        float t = (normalized - 0.2) / 0.2;
        return mix(vec3(0.0, 0.3, 1.0), vec3(0.0, 0.8, 1.0), t);
    } else if (normalized < 0.6) {
        // Medium pressure - cyan to green
        float t = (normalized - 0.4) / 0.2;
        return mix(vec3(0.0, 0.8, 1.0), vec3(0.2, 1.0, 0.2), t);
    } else if (normalized < 0.8) {
        // High pressure - green to yellow/orange
        float t = (normalized - 0.6) / 0.2;
        return mix(vec3(0.2, 1.0, 0.2), vec3(1.0, 0.9, 0.0), t);
    } else {
        // Very high pressure - orange to red
        float t = (normalized - 0.8) / 0.2;
        return mix(vec3(1.0, 0.9, 0.0), vec3(1.0, 0.0, 0.0), t);
    }
}

void main()
{
    vec3 color = pressureToColor(pressure, uMinPressure, uMaxPressure);
    FragColor = vec4(color, 0.8);
}
