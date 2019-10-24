#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 mvp;
uniform mat4 m;
uniform mat4 o2w;
uniform vec3 eye;

out VertexData
{
    vec3 view;
    vec3 normal;
} v;

void main(void)
{
    vec4 positionWorld = m * vec4(position, 1.0);
    vec4 normalWorld = o2w * vec4(normal, 0);
    v.view = normalize(eye - positionWorld.xyz);
    v.normal = normalize(normalWorld.xyz);
    gl_Position = mvp * vec4(position, 1.0);
}
