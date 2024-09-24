#include "common.hlsli"

VertexOut main(VertexIn vin)
{
    VertexOut vout;
    
    vout.positionW = vin.position;
    
    //float4 posW = mul(float4(vin.position, 1.0), objConstants.gWorld);
    
    vout.positionH = mul(float4(vin.position, 1.0), passConstants.gViewProj);
    
    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.normal = mul(vin.normal, (float3x3) objConstants.gWorld);
    
    return vout;
}