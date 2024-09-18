#include "common.hlsli"

TextureCube gCubeMap : register(t0);
Texture2D gDiffuseMap[8] : register(t1);

float4 main(VertexOut pin) : SV_TARGET
{
    return gCubeMap.Sample(gsamLinearClamp, pin.positionW);
}