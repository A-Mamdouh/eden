#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

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

layout(set = 1, binding = 0) uniform sampler2D baseColorTex;
layout(set = 2, binding = 0) uniform sampler2D metallicRoughnessTex;
layout(set = 3, binding = 0) uniform sampler2D emissiveTex;

layout(push_constant) uniform PushConstants
{
    mat4 model;
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    int useVertexColor;
    int shadingModel; // 0 = Unlit, 1 = PBR
    vec4 emissiveFactor;
} pc;

const float PI = 3.14159265359;

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-6);
}

float geometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    // Direct-light k remap (Karis).
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return geometrySchlickGGX(max(dot(N, V), 0.0), k) * geometrySchlickGGX(max(dot(N, L), 0.0), k);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 albedo = texture(baseColorTex, fragUV).rgb * fragColor;

    if (pc.shadingModel == 0) // Unlit
    {
        outColor = vec4(albedo, 1.0);
        return;
    }

    vec4 mr = texture(metallicRoughnessTex, fragUV);
    float metallic = clamp(pc.metallicFactor * mr.b, 0.0, 1.0);
    // Clamped above 0 to avoid a near-zero-roughness GGX singularity.
    float roughness = clamp(pc.roughnessFactor * mr.g, 0.045, 1.0);
    vec3 emissive = texture(emissiveTex, fragUV).rgb * pc.emissiveFactor.rgb;

    vec3 N = normalize(fragWorldNormal);
    vec3 V = normalize(frame.cameraPosition.xyz - fragWorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    int count = min(frame.lightCount.x, 16);
    for (int i = 0; i < count; ++i)
    {
        Light light = frame.lights[i];

        vec3 L;
        float attenuation = 1.0;
        if (light.positionOrDirection.w < 0.5) // Directional
        {
            L = normalize(-light.positionOrDirection.xyz);
        }
        else // Point
        {
            vec3 toLight = light.positionOrDirection.xyz - fragWorldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 1e-4);
            attenuation = 1.0 / max(dist * dist, 1e-4);

            float range = light.params.x;
            if (range > 0.0)
            {
                // KHR_lights_punctual-style smooth range cutoff.
                float falloff = clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0);
                attenuation *= falloff * falloff;
            }
        }

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0)
        {
            continue;
        }

        vec3 H = normalize(V + L);
        vec3 radiance = light.colorIntensity.rgb * light.colorIntensity.a * attenuation;

        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 specular = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * NdotL + 1e-4);
        // Metals have no diffuse term; F already accounts for the
        // dielectric/metal split via F0's mix() above.
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = frame.ambientColor.rgb * frame.ambientColor.a * albedo;
    outColor = vec4(ambient + Lo + emissive, 1.0);
}
