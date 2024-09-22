#include "common.hlsli"

TextureCube gCubeMap : register(t0);
TextureCube prefilterMap : register(t1);
Texture2D gDiffuseMap[8] : register(t2);

float4 main(VertexOut pin) : SV_TARGET
{
    
    float3 color = gCubeMap.SampleLevel(gsamLinearClamp, pin.positionW, 0.0).rgb;
    
    // HDR tonemap and gamma correct
   
    return float4(color, 1.0);
}