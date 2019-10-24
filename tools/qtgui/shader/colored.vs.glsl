#version 410 core

layout(location = 0) in vec3 position;
layout(location = 3) in vec3 color;

out VertexData
{
    vec3 color;
} v;

uniform mat4 mvp;

void main(void)
{
    v.color = color;
    gl_Position = mvp * vec4(position, 1.0);
}
