#include "common.hlsli"

TextureCube gCubeMap : register(t0);
TextureCube prefilterMap : register(t1);
Texture2D gDiffuseMap[8] : register(t2);

float4 main(VertexOut pin) : SV_TARGET
{
    
    float3 color = gCubeMap.Sample(gsamLinearClamp, pin.positionW).rgb;
    
    // HDR tonemap and gamma correct
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, 1.0 / 2.2);
    
    return float4(color, 1.0);
}