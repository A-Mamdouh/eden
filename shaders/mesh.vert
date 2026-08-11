#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

layout(push_constant) uniform PushConstants
{
    mat4 mvp;
    vec4 color;
    int useVertexColor;
} pc;

void main()
{
    vec3 baseColor = pc.useVertexColor != 0 ? inColor : pc.color.rgb;
    fragColor = baseColor;
    fragUV = inUV;
    gl_Position = pc.mvp * vec4(inPos, 1.0);
}
