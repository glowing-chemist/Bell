#define DIELECTRIC_SPECULAR 0.04

#include "Hammersley.glsl"

// polynomial aproximation of a monte carlo integrated DFG function.
// Implementation taken from https://knarkowicz.wordpress.com/2014/12/27/analytical-dfg-term-for-ibl/
// gloss=(1-roughness)^4 
vec3 analyticalDFG( vec3 specularColor, float gloss, float ndotv )
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


vec3 ImportanceSampleGGX(const vec2 Xi, const float roughness, const vec3 N)
{
    float a = roughness*roughness;
    
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}


vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}


vec3 calculateDiffuse(vec3 dA, float M, vec3 irradiance)
{
    const vec3 diffuseColor = dA * (1.0 - DIELECTRIC_SPECULAR) * (1.0 - M);

    return diffuseColor * irradiance;
}


vec3 calculateDiffuseLambert(vec3 dA, float M, vec3 irradiance)
{
    const vec3 diffuseColor = dA * (1.0 - DIELECTRIC_SPECULAR) * (1.0 - M);

    return (diffuseColor * irradiance) / PI;
}


vec3 calculateSpecular(const float R, const vec3 N, const vec3 V, const float M, const vec3 dA, const vec3 radiance, const vec2 DFG)
{
    const float NoV = dot(N, V);

    const vec3 F0 = mix(vec3(DIELECTRIC_SPECULAR), dA, M);
    const vec3 F = fresnelSchlickRoughness(max(NoV, 0.0), F0, R);

    return (F * DFG.x + DFG.y) * radiance;
}
