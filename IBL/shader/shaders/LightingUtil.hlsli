#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "common.hlsli"

#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 1
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

struct Material
{
    float4 gDiffuseAlbedo;
    float3 F0;
    float gRoughness;
    float Metallic;
};

static const float PI = 3.14159265359;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 SchlickFresnel(float3 R0, float cosTheta)
{
    float f0 = saturate(1.0f - cosTheta);

    return R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light light, Material mat, float3 normal, float3 V, float3 pos)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 L = normalize(light.Position - pos);
    float3 H = normalize(V + L);
    
    float roughness = 0.8;
    
    // cook-torrance brdf
    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, V, L, roughness);
    float3 F = SchlickFresnel(mat.F0, max(dot(H, V), 0.0));
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = 1.0 - kS;
    
    kD *= 1.0 - mat.Metallic;
    
    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(L, normal), 0.0f);
    float3 lightStrength = light.Strength * ndotl;
    
    return (kD * mat.gDiffuseAlbedo.rgb / PI + specular) * lightStrength;
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye)
{
    float3 result = 0.0f;

    int i = 0;

#if (NUM_POINT_LIGHTS > 0)
    for (i = 0; i < NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, normal, toEye, pos);
    }
#endif
    
    return float4(result, 0.0f);
}

#endif // LIGHTING_HLSLI