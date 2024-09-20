#include "common.hlsli"

TextureCube environmentMap : register(t0);
Texture2D gDiffuseMap[8] : register(t1);

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

// 单位圆上生成均匀分布的点，
float2 UniformOnDisk(float Xi)
{
    float theta = 2 * PI * Xi;
    return float2(cos(theta), sin(theta));
}

// 余弦加权分布的采样方向
float3 CosOnHalfSphere(float2 Xi)
{
    float r = sqrt(Xi.x);
    float2 pInDisk = r * UniformOnDisk(Xi.y);
    float z = sqrt(1 - Xi.x);
    return float3(pInDisk, z);
}

float4 Quat_ZTo(float3 to)
{
	// from = (0, 0, 1)
	//float cosTheta = dot(from, to);
	//float cosHalfTheta = sqrt(max(0, (cosTheta + 1) * 0.5));
    float cosHalfTheta = sqrt(max(0, (to.z + 1) * 0.5));
	//vec3 axisSinTheta = cross(from, to);
	//    0    0    1
	// to.x to.y to.z
	//vec3 axisSinTheta = vec3(-to.y, to.x, 0);
    float twoCosHalfTheta = 2 * cosHalfTheta;
    return float4(-to.y / twoCosHalfTheta, to.x / twoCosHalfTheta, 0, cosHalfTheta);
}

float4 Quat_Inverse(float4 q)
{
    return float4(-q.xyz, q.w);
}

float3 Quat_Rotate(float4 q, float3 p)
{
	// Quat_Mul(Quat_Mul(q, vec4(p, 0)), Quat_Inverse(q)).xyz;

    float4 qp = float4(q.w * p + cross(q.xyz, p), -dot(q.xyz, p));
    float4 invQ = Quat_Inverse(q);
    float3 qpInvQ = qp.w * invQ.xyz + invQ.w * qp.xyz + cross(qp.xyz, invQ.xyz);
    return qpInvQ;
}

float3 CosOnHalfSphere(float2 Xi, float3 N)
{
    // tangent space变换到world space，对齐法线N
    float3 p = CosOnHalfSphere(Xi);
    float4 rot = Quat_ZTo(N);
    return Quat_Rotate(rot, p);
}

float4 main(VertexOut pin) : SV_TARGET
{
    float3 N = normalize(pin.positionW);
    float3 irradiance = float3(0.0, 0.0, 0.0);
    
    // tangent space calculation from origin point
    //float3 up = float3(0.0, 1.0, 0.0);
    //float3 right = normalize(cross(up, N));
    //up = normalize(cross(N, right));
    //
    //float sampleDelta = 0.15;
    //float nrSamples = 0.0;
    //for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    //{
    //    for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
    //    {
    //        // spherical to cartesian (in tangent space)
    //        float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    //        // tangent space to world
    //        float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
    //
    //        irradiance += environmentMap.Sample(gsamLinearClamp, sampleVec).rgb * cos(theta) * sin(theta);
    //        nrSamples++;
    //    }
    //}
    //
    //irradiance = PI * irradiance * (1.0 / float(nrSamples));
    
    const int SAMPLE_NUM = 64;
    
    for (int i = 0; i < SAMPLE_NUM; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_NUM);
        float3 L = CosOnHalfSphere(Xi, N);
        irradiance += environmentMap.Sample(gsamLinearClamp, L);
    }
    
    irradiance = PI * irradiance * (1.0 / float(SAMPLE_NUM));
    
    return float4(irradiance, 1.0);
}