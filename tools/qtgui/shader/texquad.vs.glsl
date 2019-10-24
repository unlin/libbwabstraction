#version 410 core

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texcoord;

out VertexData
{
    vec2 texcoord;
} v;

uniform mat4 mvp;

void main(void)
{
    v.texcoord = texcoord;
    gl_Position = mvp * vec4(position, 1.0);
}
