#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "common.hlsli"

#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 2
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

static const float PI1 = 3.14159265359;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI1 * denom * denom;
    
    return 1.0;
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
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 SchlickFresnel(float3 R0, float cosTheta)
{
    float f0 = saturate(1.0f - cosTheta);

    return R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
}

float3 CT(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;
    
    return CT(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light L, Material mat, float3 normal, float3 toEye, float3 pos)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = L.Position - pos;
    float3 H = normalize(toEye + lightVec);
    
    float roughness = mat.gRoughness;
    
    // cook-torrance brdf
    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, toEye, lightVec, roughness);
    float3 F = SchlickFresnel(mat.F0, max(dot(H, toEye), 0.0));
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, toEye), 0.0) * max(dot(normal, lightVec), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = 1.0 - kS;
    
    kD *= 1.0 - mat.Metallic;
    
    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;
    
    return (kD * mat.gDiffuseAlbedo.rgb / PI1 + specular) * lightStrength;
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye)
{
    float3 result = 0.0f;

    int i = 0;

#if (NUM_DIR_LIGHTS > 0)
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result +=  ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for (i = 0; i < NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, normal, toEye, pos);
    }
#endif
    
    return float4(result, 0.0f);
}

#endif // LIGHTING_HLSLI