#version 410 core

layout(location = 0) out int primitiveIDOut;

void main(void)
{
    primitiveIDOut = gl_PrimitiveID;
}
