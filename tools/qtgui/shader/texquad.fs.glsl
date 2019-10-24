#version 410 core

layout(location = 0) out vec4 fragColor;

uniform sampler2D tex;

in VertexData
{
    vec2 texcoord;
} v;

void main(void)
{
    fragColor = texture(tex, v.texcoord);
}
