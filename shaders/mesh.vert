#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragWorldNormal;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec2 fragUV;

struct Light
{
    vec4 positionOrDirection; // xyz; w = 0 (directional) / 1 (point)
    vec4 colorIntensity;      // rgb = color, a = intensity
    vec4 params;              // x = range, yzw reserved
};

layout(std140, set = 0, binding = 0) uniform FrameUBO
{
    mat4 view;
    mat4 proj;
    vec4 cameraPosition;
    vec4 ambientColor;
    ivec4 lightCount;   // x = count
    Light lights[16];   // must match Eden::Rendering::kMaxLights
} frame;

layout(push_constant) uniform PushConstants
{
    mat4 model;
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    int useVertexColor;
    int shadingModel;
    vec4 emissiveFactor;
} pc;

void main()
{
    vec4 worldPos = pc.model * vec4(inPos, 1.0);
    fragWorldPos = worldPos.xyz;
    fragWorldNormal = mat3(transpose(inverse(pc.model))) * inNormal;
    // Unified "flat multiplier": Unlit reads this as vertex-color-or-tint
    // (today's behavior); PBR reads it as baseColorFactor, letting the
    // fragment shader use fragColor as the albedo multiplier either way.
    fragColor = pc.useVertexColor != 0 ? inColor : pc.baseColorFactor.rgb;
    fragUV = inUV;
    gl_Position = frame.proj * frame.view * worldPos;
}
