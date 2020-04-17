
#include "Hammersley.hlsl"

#define    kMaterial_Albedo 1
#define    kMaterial_Diffuse (1 << 1)
#define    kMaterial_Normals (1 << 2)
#define    kMaterial_Roughness (1 << 3)
#define    kMaterial_Gloss (1 << 4)
#define    kMaterial_Metalness (1 << 5)
#define    kMaterial_Specular (1 << 6)

struct MaterialInfo
{
    float4 diffuse;
    float4 specularRoughness;
    float4 normal;
};

// polynomial aproximation of a monte carlo integrated DFG function.
// Implementation taken from https://knarkowicz.wordpress.com/2014/12/27/analytical-dfg-term-for-ibl/
// gloss=(1-roughness)^4 
float3 analyticalDFG( float3 specularColor, float gloss, float ndotv )
{
    float x = gloss;
    float y = ndotv;
 
    float b1 = -0.1688;
    float b2 = 1.895;
    float b3 = 0.9903;
    float b4 = -4.853;
    float b5 = 8.404;
    float b6 = -5.069;
    float bias = clamp( min( b1 * x + b2 * x * x, b3 + b4 * y + b5 * y * y + b6 * y * y * y ), 0.0, 1.0 );
 
    float d0 = 0.6045;
    float d1 = 1.699;
    float d2 = -0.5228;
    float d3 = -3.603;
    float d4 = 1.404;
    float d5 = 0.1939;
    float d6 = 2.661;
    float delta = clamp( d0 + d1 * x + d2 * y + d3 * x * x + d4 * x * y + d5 * y * y + d6 * x * x * x , 0.0, 1.0);
    float scale = delta - bias;
 
    bias *= clamp( 50.0 * specularColor.y , 0.0, 1.0);
    return specularColor * scale + bias;
}


float3 ImportanceSampleGGX(const float2 Xi, const float roughness, const float3 N)
{
    float a = roughness*roughness;
    
    float phi = 2.0 * PI * Xi.x;
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


float3 calculateDiffuse(const MaterialInfo material, const float3 irradiance)
{
    return material.diffuse * irradiance;
}


float3 calculateDiffuseLambert(const MaterialInfo material, const float3 irradiance)
{

    return (material.diffuse * irradiance) / PI;
}


float3 calculateSpecular(const MaterialInfo material, const float3 radiance, const float2 DFG)
{
    return (material.specularRoughness.xyz * DFG.x + DFG.y) * radiance;
}
