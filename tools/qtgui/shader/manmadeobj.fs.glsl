#version 410 core

in VertexData
{
    vec3 view;
    vec3 normal;
} v;

uniform vec3 light;

layout(location = 0) out vec4 fragColor;

void main()
{
    const vec3 lightColor = vec3(1.0, 1.0, 1.0);
    const vec3 matDiffuse = vec3(0.55, 0.55, 0.55);
    const vec3 matSpecular = vec3(0.7, 0.7, 0.7);
    const vec3 matAmbient = vec3(0.3, 0.3, 0.3);
    const float matShiness = 25.0;

    vec3 N = normalize(v.normal);
    vec3 V = normalize(v.view);
    vec3 L = normalize(light);
    vec3 H = normalize(V + L);

    vec3 diffuse = clamp(dot(N, L), 0.0, 1.0) * matDiffuse;
    //vec3 specular = pow(clamp(dot(N, H), 0.0, 1.0), matShiness) * matSpecular;
    vec3 intensity = matAmbient * 1.2 + diffuse/* + specular*/;

    fragColor = vec4(intensity * lightColor, 1.0);
}
