#include "common.hlsli"

VertexOut main(VertexIn vin)
{
    VertexOut vout;
    
    vout.positionW = vin.position;
    
    //float4 posW = mul(float4(vin.position, 1.0), objConstants.gWorld);
    
    vout.positionH = mul(float4(vin.position, 1.0), passConstants.gViewProj);
    
    return vout;
}