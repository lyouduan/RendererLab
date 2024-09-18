#include "common.hlsli"

TextureCube environmentMap : register(t0);
Texture2D gDiffuseMap[8] : register(t1);

const float PI = 3.14159265359;

float4 main(VertexOut pin) : SV_TARGET
{
    float3 N = normalize(pin.positionW);
    float3 irradiance = float3(0.0, 0.0, 0.0);
    
    // tangent space calculation from origin point
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));
    
    float sampleDelta = 0.05;
    float nrSamples = 0.0;
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += environmentMap.Sample(gsamLinearClamp, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    //irradiance = pow(irradiance.rgb, 1/2.2);
    
    return float4(irradiance, 1.0);
}