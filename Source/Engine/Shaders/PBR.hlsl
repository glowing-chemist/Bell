
#include "Hammersley.hlsl"

#define    kMaterial_Albedo (1)
#define    kMaterial_Diffuse (1 << 1)
#define    kMaterial_Normals (1 << 2)
#define    kMaterial_Roughness (1 << 3)
#define    kMaterial_Gloss (1 << 4)
#define    kMaterial_Metalness (1 << 5)
#define    kMaterial_Specular (1 << 6)
#define    kMaterial_CombinedMetalnessRoughness (1 << 7)
#define    kMaterial_AmbientOcclusion (1 << 8)
#define    kMaterial_Emissive (1 << 9)


struct MaterialInfo
{
    float4 diffuse;
    float4 specularRoughness; // xyz specular w roughness
    float4 normal;
    float4 emissiveOcclusion; // xyz emisive w ambient occlusion.
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


void  importanceSampleCosDir(in float2 u, in float3 N, out float3 L, out  float NdotL)
{
    //  Local  referencial
    float3  upVector = abs(N.z) < 0.999 ? float3 (0,0,1) : float3 (1,0,0);
    float3  tangentX = normalize( cross( upVector , N ) );
    float3  tangentY = cross( N, tangentX );

    float  u1 = u.x;
    float  u2 = u.y;

    float r = sqrt(u1);
    float  phi = u2 * PI * 2;

    L = float3(r*cos(phi), r*sin(phi), sqrt(max (0.0f,1.0f-u1)));
    L = normalize(tangentX * L.y + tangentY * L.x + N * L.z);

    NdotL = dot(L,N);
}


// Moving frostbite to PBR.
float3 F_Schlick(in float3 f0, in float f90, in float u)
{
    return  f0 + (f90 - f0) * pow (1.f - u, 5.f);
}


float Fr_DisneyDiffuse(float  NdotV , float  NdotL , float  LdotH ,float linearRoughness)
{
    float  energyBias = lerp(0, 0.5,  linearRoughness);
    float  energyFactor = lerp (1.0, 1.0 / 1.51,  linearRoughness);
    float  fd90 = energyBias + 2.0 * LdotH*LdotH * linearRoughness;
    float3  f0 = float3 (1.0f, 1.0f, 1.0f);
    float  lightScatter   = F_Schlick(f0, fd90 , NdotL).r;
    float  viewScatter    = F_Schlick(f0, fd90 , NdotV).r;

    return  lightScatter * viewScatter * energyFactor;
}


float3 calculateDiffuse(const MaterialInfo material, const float3 irradiance)
{
    return material.diffuse * irradiance;
}


float3 calculateDiffuseLambert(const MaterialInfo material, const float3 irradiance)
{

    return (material.diffuse * irradiance) / PI;
}

float3 calculateDiffuseDisney(const MaterialInfo material, const float3 irradiance, const float3 DFG)
{
    return material.diffuse * DFG.z * irradiance;
}


float3 calculateSpecular(const MaterialInfo material, const float3 radiance, const float3 DFG)
{
    return (material.specularRoughness.xyz * DFG.x + DFG.y) * radiance;
}
