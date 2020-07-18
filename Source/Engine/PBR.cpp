#include "PBR.hpp"

void  importanceSampleCosDir(const float2& u, const float3& N, float3& L, float& NdotL)
{
    //  Local  referencial
    float3  upVector = abs(N.z) < 0.999 ? float3 (0,0,1) : float3 (1,0,0);
    float3  tangentX = normalize( cross( upVector , N ) );
    float3  tangentY = cross( N, tangentX );

    float  u1 = u.x;
    float  u2 = u.y;

    float r = sqrt(u1);
    float  phi = u2 * M_PI * 2;

    L = float3(r*cos(phi), r*sin(phi), sqrt(std::max(0.0f,1.0f-u1)));
    L = normalize(tangentX * L.y + tangentY * L.x + N * L.z);

    NdotL = dot(L,N);
}

float3 ImportanceSampleGGX(const float2& Xi, const float roughness, const float3& N)
{
    float a = roughness*roughness;

    float phi = 2.0 * M_PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    float3 up        = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}


float3 hemisphereSample_uniform(float u, float v)
{
    float phi = v * 2.0 * M_PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}


float3 hemisphereSample_cos(float u, float v)
{
    float phi = v * 2.0 * M_PI;
    float cosTheta = sqrt(1.0 - u);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}


float2 Hammersley(uint i, uint N)
{
    return float2(float(i)/float(N), RadicalInverse_VdC(i));
}


float3 Hamersley_uniform(uint i, uint N)
{
    float2 ham = Hammersley(i, N);
    return hemisphereSample_uniform(ham.x, ham.y);
}


float3 Hamersley_cosine(uint i, uint N)
{
    float2 ham = Hammersley(i, N);
    return hemisphereSample_cos(ham.x, ham.y);
}

float3 F_Schlick(const float3 f0, const float f90, const float u)
{
    return  f0 + (f90 - f0) * std::pow (1.0f - u, 5.0f);
}

float disneyDiffuse(float  NdotV , float  NdotL , float  LdotH ,float linearRoughness)
{
    float  energyBias = glm::lerp(0.0f, 0.5f,  linearRoughness);
    float  energyFactor = glm::lerp (1.0f, 1.0f / 1.51f,  linearRoughness);
    float  fd90 = energyBias + 2.0f * LdotH*LdotH * linearRoughness;
    float3  f0 = float3 (1.0f, 1.0f, 1.0f);
    float  lightScatter   = F_Schlick(f0, fd90 , NdotL).r;
    float  viewScatter    = F_Schlick(f0, fd90 , NdotV).r;

    return  lightScatter * viewScatter * energyFactor;
}

float specular_GGX(const float3& N, const float3& V, const float3& L, const float roughness, const float3& F0)
{
    const float3 H = glm::normalize(V + L);

    const float NdotL = glm::clamp(glm::dot(N, L), 0.0f, 1.0f);
    const float NdotV = glm::clamp(glm::dot(N, V), 0.0f, 1.0f);
    const float NdotH = glm::clamp(glm::dot(N, H), 0.0f, 1.0f);
    const float LdotH = glm::clamp(glm::dot(L, H), 0.0f, 1.0f);

    float a = NdotH * roughness;
    float k = roughness / (1.0f - NdotH * NdotH + a * a);
    const float D =  k * k * (1.0f / M_PI);

    const float3 F = F_Schlick(F0, 1.0f, LdotH);

    float a2 = roughness * roughness;
    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0f - a2) + a2);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0f - a2) + a2);
    const float G =  0.5f / (GGXV + GGXL);

    const float result = (NdotL * D * F.x * G) / (4.0f * glm::clamp(glm::dot(N, V), 0.0f, 1.0f) * glm::clamp(glm::dot(N, L), 0.0f, 1.0f));
    return glm::isnan(result) ? 1.0f : glm::clamp(result, 0.0f, 1.0f);
}
