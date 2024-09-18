#include "common.hlsli"

VertexOut main(VertexIn input) 
{
    VertexOut output;
    
    //MaterialData matData = gMaterialData[objConstants.gMaterialIndex];
    
    float4 posW = mul(float4(input.position, 1.0), objConstants.gWorld);
    output.positionW = posW.xyz;
    output.positionH = mul(posW, passConstants.gViewProj);
    
    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    output.normal = mul(input.normal, (float3x3)objConstants.gWorld);
    
    float4 tex = mul(float4(input.tex, 0.0, 1.0), objConstants.gTexTransform);
    output.tex = mul(tex, objConstants.gMatTransform).xy;
    
    return output;
}

