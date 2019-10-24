#version 410 core

layout(location = 0) out vec4 fragColor;

in VertexData
{
    vec3 color;
} v;

void main(void)
{
    fragColor = vec4(v.color, 1.0);
}
