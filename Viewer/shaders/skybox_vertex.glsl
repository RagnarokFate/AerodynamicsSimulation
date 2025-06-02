#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    
    // Remove translation from the view matrix by only using the rotation part
    mat4 rotView = mat4(mat3(view));
    vec4 pos = projection * rotView * vec4(aPos, 1.0);
    
    // Trick to make sure the skybox is always behind everything else
    gl_Position = pos.xyww;
}