#include "common.hlsli"

Texture2D gCubeMap : register(t0);
Texture2D gDiffuseMap[8] : register(t1);

#define PI 3.1415

float2 SphereToEquirectangular(float3 dir)
{
    float phi = atan2(dir.z, dir.x);
    float theta = acos(dir.y);
    
    float u = (phi + PI) / (2.0 * PI);
    float v = theta / PI;
    
    return float2(u, v);
}


float4 main(VertexOut pin) : SV_TARGET
{
    float2 uv = SphereToEquirectangular(normalize(pin.positionW));
    // dx坐标系在左上角， openGL在左下角
    uv.y = 1 - uv.y;
    float3 color = gCubeMap.Sample(gsamLinearClamp, uv).rgb;
    return float4(color, 1.0f);
}