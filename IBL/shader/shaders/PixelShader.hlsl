#include "common.hlsli"
#include "LightingUtil.hlsli"

TextureCube gIrradianceCubeMap : register(t0);
Texture2D gDiffuseMap[8] : register(t1);

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float4 main(VertexOut input) : SV_TARGET
{
    MaterialData matData = gMaterialData[objConstants.gMaterialIndex];
    float4 diffuseAlbedo = matData.gDiffuseAlbedo;
    float3 fresnelR0 = matData.gFresnelR0;
    float roughness = matData.gRoughness;
    
    uint diffuseMapIndex = matData.gDiffuseMapIndex;
    
    diffuseAlbedo *= gDiffuseMap[diffuseMapIndex].Sample(gsamLinearClamp, input.tex);
// alpha tested
    clip(diffuseAlbedo.a - 0.1f);
    
    input.normal = normalize(input.normal);
    
    float3 toEyeW = passConstants.gEyePosW - input.positionW;
    float distance = length(toEyeW);
    toEyeW /= distance; // normalize
    
    const float shininess = 1.0f - roughness;
    
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(passConstants.Lights, mat, input.positionW,
        input.normal, toEyeW, shadowFactor);
    
    
    // ambient lighting (use IBL as the ambient term)
    
    float3 Ks = fresnelSchlick(max(dot(input.normal, toEyeW), 0.0), fresnelR0);
    float3 Kd = 1.0 - Ks;
    float3 irradiance = gIrradianceCubeMap.Sample(gsamLinearClamp, input.normal).rgb;
    float3 diffuse = irradiance * diffuseAlbedo.rgb;
    float3 ambient = Kd * diffuse * passConstants.gAmbientLight.rgb;
    
    float3 litColor = ambient + directLight.rgb;
    
    // HDR tonemap and gamma correct
    litColor = litColor / (litColor + float3(1.0, 1.0, 1.0));
    litColor = pow(litColor, 1.0 / 2.2);
    
    return float4(litColor, diffuseAlbedo.a);
}