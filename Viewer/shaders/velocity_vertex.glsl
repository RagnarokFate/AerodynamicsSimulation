#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aVelocity;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uVelocityScale;

out vec3 velocity;
out float velocityMagnitude;

void main()
{
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    velocity = aVelocity;
    velocityMagnitude = length(aVelocity);
}
