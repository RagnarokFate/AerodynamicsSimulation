#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D inputTexture;
uniform vec2 texelSize;
uniform bool horizontal;
uniform float blurRadius;
uniform float blurStrength;

// Gaussian weights for blur kernel
const float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

void main()
{
    vec3 result = texture(inputTexture, TexCoord).rgb * weight[0];
    
    if (horizontal) {
        // Horizontal blur pass
        for(int i = 1; i < 5; ++i) {
            float offset = float(i) * blurRadius;
            result += texture(inputTexture, TexCoord + vec2(texelSize.x * offset, 0.0)).rgb * weight[i];
            result += texture(inputTexture, TexCoord - vec2(texelSize.x * offset, 0.0)).rgb * weight[i];
        }
    } else {
        // Vertical blur pass
        for(int i = 1; i < 5; ++i) {
            float offset = float(i) * blurRadius;
            result += texture(inputTexture, TexCoord + vec2(0.0, texelSize.y * offset)).rgb * weight[i];
            result += texture(inputTexture, TexCoord - vec2(0.0, texelSize.y * offset)).rgb * weight[i];
        }
    }
    
    // Mix original with blurred based on blur strength
    vec3 original = texture(inputTexture, TexCoord).rgb;
    FragColor = vec4(mix(original, result, blurStrength), 1.0);
}
