#include "common.hlsli"
#include "LightingUtil.hlsli"

TextureCube gIrradianceCubeMap : register(t0);
TextureCube prefilterMap : register(t1);
Texture2D gDiffuseMap[8] : register(t2);

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float4 main(VertexOut input) : SV_TARGET
{
    MaterialData matData = gMaterialData[objConstants.gMaterialIndex];
    float4 diffuseAlbedo = matData.gDiffuseAlbedo;
    float metallic = matData.gMetallic;
    float roughness = matData.gRoughness;
    
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, diffuseAlbedo.rgb, metallic);
    
    float3 normal = normalize(input.normal);
    float3 toEyeW = normalize(passConstants.gEyePosW - input.positionW);
    
    Material mat = { diffuseAlbedo, F0, roughness, metallic};
    
    Light light = passConstants.Lights[0];
    
    float3 directLight = { 0.0, 0.0, 0.0 };
    {
        // The light vector aims opposite the direction the light rays travel.
        float3 lightVec = light.Position - input.positionW;
        float3 H = normalize(toEyeW + lightVec);
    
        // cook-torrance brdf
        float NDF = DistributionGGX(normal, H, roughness);
        float G = GeometrySmith(normal, toEyeW, lightVec, roughness);
        float3 F = SchlickFresnel(F0, max(dot(H, toEyeW), 0.0));
    
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, toEyeW), 0.0) * max(dot(normal, lightVec), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        float3 specular = numerator / denominator;

        float3 kS = F;
        float3 kD = 1.0 - kS;
    
        kD *= 1.0 - mat.Metallic;
        // Scale light down by Lambert's cosine law.
        float ndotl = max(dot(lightVec, normal), 0.0f);
        float3 lightStrength = light.Strength * ndotl;
    
        directLight = (kD * diffuseAlbedo.rgb / 3.1415 + specular) * lightStrength;
    }
    
    // ambient lighting (use IBL as the ambient term)
    float3 F = fresnelSchlickRoughness(max(dot(normal, toEyeW), 0.0), F0, roughness);
    
    float3 Ks = F;
    float3 Kd = 1.0 - Ks;

    float3 irradiance = gIrradianceCubeMap.Sample(gsamLinearClamp, normal).rgb;
    float3 diffuse = irradiance * diffuseAlbedo.rgb;
    
    float3 R = reflect(-toEyeW, normal);
    
    const float MAX_LOD = 4.0;
    
    float3 prefilteredColor = prefilterMap.SampleLevel(gsamLinearClamp, R, roughness * MAX_LOD).rgb;
    
    float2 scale_bias = gDiffuseMap[0].Sample(gsamLinearClamp, float2(saturate(dot(normal, toEyeW)), roughness)).rg;
    
    float3 specular = prefilteredColor * (F * scale_bias.x + scale_bias.y);
    
    float3 ambient = (Kd * diffuse + specular) * passConstants.gAmbientLight.rgb;
    
    float3 litColor = ambient + directLight;
    
    // HDR tonemap and gamma correct
    litColor = litColor / (litColor + float3(1.0, 1.0, 1.0));
    litColor = pow(litColor, 1.0 / 2.2);
    
    return float4(litColor, 1.0);
}