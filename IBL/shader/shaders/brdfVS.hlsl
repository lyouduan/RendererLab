#include "common.hlsli"

VertexOut main(VertexIn vin)
{
    VertexOut vout;
    
    vout.tex = vin.tex;
    vout.positionH = float4(vin.position, 1.0);
    
    return vout;
}