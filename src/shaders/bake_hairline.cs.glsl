#version 450 core

layout(binding = 0) uniform isampler2D inputMap;
layout(binding = 1) uniform isampler2D patchIDMap;

layout(rgba8, binding = 0) uniform image2D outputMap;

layout(location = 0) uniform int radius;

layout(std430, binding = 0) buffer Label
{
    int labels[];
};

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main(void)
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    bool found = false;
    int pid = texelFetch(patchIDMap, pos, 0).x;
    float radius_f = float(radius);
    if(pid < 0)
    {
        return;
    }
    for(int i = -radius; i <= radius; ++i)
    {
        for(int j = -radius; j <= radius; ++j)
        {
            ivec2 diff = ivec2(i, j);
            ivec2 pos2 = pos + diff;
            float diff_len = length(diff);
            int linePid = texelFetch(inputMap, pos2, 0).x;
            // boundary line
            if(diff_len <= radius_f && linePid == pid)
            {
                found = true;
                break;
            }
            // feature line
            else if(diff_len <= radius_f * 0.5 && linePid == -pid - 1)
            {
                found = true;
                break;
            }
        }
    }
    if(bool(labels[pid]) ^^ found)
    {
        imageStore(outputMap, pos, vec4(vec3(1.0), 1.0));
    }
    else
    {
        imageStore(outputMap, pos, vec4(vec3(0.0), 1.0));
    }
}
