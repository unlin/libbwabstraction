const char* colored_fs_glsl = 
"#version 410 core                        \n"
"                                         \n"
"layout(location = 0) out vec4 fragColor; \n"
"                                         \n"
"in VertexData                            \n"
"{                                        \n"
"    vec3 color;                          \n"
"} v;                                     \n"
"                                         \n"
"void main(void)                          \n"
"{                                        \n"
"    fragColor = vec4(v.color, 1.0);      \n"
"}                                        \n";

const char* colored_vs_glsl = 
"#version 410 core                            \n"
"                                             \n"
"layout(location = 0) in vec3 position;       \n"
"layout(location = 3) in vec3 color;          \n"
"                                             \n"
"out VertexData                               \n"
"{                                            \n"
"    vec3 color;                              \n"
"} v;                                         \n"
"                                             \n"
"uniform mat4 mvp;                            \n"
"                                             \n"
"void main(void)                              \n"
"{                                            \n"
"    v.color = color;                         \n"
"    gl_Position = mvp * vec4(position, 1.0); \n"
"}                                            \n";

const char* manmadeobj_fs_glsl = 
"#version 410 core                                                                \n"
"                                                                                 \n"
"in VertexData                                                                    \n"
"{                                                                                \n"
"    vec3 view;                                                                   \n"
"    vec3 normal;                                                                 \n"
"} v;                                                                             \n"
"                                                                                 \n"
"uniform vec3 light;                                                              \n"
"                                                                                 \n"
"layout(location = 0) out vec4 fragColor;                                         \n"
"                                                                                 \n"
"void main()                                                                      \n"
"{                                                                                \n"
"    const vec3 lightColor = vec3(1.0, 1.0, 1.0);                                 \n"
"    const vec3 matDiffuse = vec3(0.55, 0.55, 0.55);                              \n"
"    const vec3 matSpecular = vec3(0.7, 0.7, 0.7);                                \n"
"    const vec3 matAmbient = vec3(0.3, 0.3, 0.3);                                 \n"
"    const float matShiness = 25.0;                                               \n"
"                                                                                 \n"
"    vec3 N = normalize(v.normal);                                                \n"
"    vec3 V = normalize(v.view);                                                  \n"
"    vec3 L = normalize(light);                                                   \n"
"    vec3 H = normalize(V + L);                                                   \n"
"                                                                                 \n"
"    vec3 diffuse = clamp(dot(N, L), 0.0, 1.0) * matDiffuse;                      \n"
"    //vec3 specular = pow(clamp(dot(N, H), 0.0, 1.0), matShiness) * matSpecular; \n"
"    vec3 intensity = matAmbient * 1.2 + diffuse/* + specular*/;                  \n"
"                                                                                 \n"
"    fragColor = vec4(intensity * lightColor, 1.0);                               \n"
"}                                                                                \n";

const char* manmadeobj_vs_glsl = 
"#version 410 core                                 \n"
"                                                  \n"
"layout(location = 0) in vec3 position;            \n"
"layout(location = 1) in vec3 normal;              \n"
"                                                  \n"
"uniform mat4 mvp;                                 \n"
"uniform mat4 m;                                   \n"
"uniform mat4 o2w;                                 \n"
"uniform vec3 eye;                                 \n"
"                                                  \n"
"out VertexData                                    \n"
"{                                                 \n"
"    vec3 view;                                    \n"
"    vec3 normal;                                  \n"
"} v;                                              \n"
"                                                  \n"
"void main(void)                                   \n"
"{                                                 \n"
"    vec4 positionWorld = m * vec4(position, 1.0); \n"
"    vec4 normalWorld = o2w * vec4(normal, 0);     \n"
"    v.view = normalize(eye - positionWorld.xyz);  \n"
"    v.normal = normalize(normalWorld.xyz);        \n"
"    gl_Position = mvp * vec4(position, 1.0);      \n"
"}                                                 \n";

const char* texquad_fs_glsl = 
"#version 410 core                         \n"
"                                          \n"
"layout(location = 0) out vec4 fragColor;  \n"
"                                          \n"
"uniform sampler2D tex;                    \n"
"                                          \n"
"in VertexData                             \n"
"{                                         \n"
"    vec2 texcoord;                        \n"
"} v;                                      \n"
"                                          \n"
"void main(void)                           \n"
"{                                         \n"
"    fragColor = texture(tex, v.texcoord); \n"
"}                                         \n";

const char* texquad_vs_glsl = 
"#version 410 core                            \n"
"                                             \n"
"layout(location = 0) in vec3 position;       \n"
"layout(location = 2) in vec2 texcoord;       \n"
"                                             \n"
"out VertexData                               \n"
"{                                            \n"
"    vec2 texcoord;                           \n"
"} v;                                         \n"
"                                             \n"
"uniform mat4 mvp;                            \n"
"                                             \n"
"void main(void)                              \n"
"{                                            \n"
"    v.texcoord = texcoord;                   \n"
"    gl_Position = mvp * vec4(position, 1.0); \n"
"}                                            \n";

