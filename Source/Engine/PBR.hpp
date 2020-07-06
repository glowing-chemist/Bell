#ifndef PBR_HPP
#define PBR_HPP

#include "Engine/GeomUtils.h"

void  importanceSampleCosDir(const float2& u, const float3& N, float3& L, float& NdotL);

float3 ImportanceSampleGGX(const float2 Xi, const float roughness, const float3 N);

float RadicalInverse_VdC(uint bits);


float3 hemisphereSample_uniform(float u, float v);


float3 hemisphereSample_cos(float u, float v);


float2 Hammersley(uint i, uint N);


float3 Hamersley_uniform(uint i, uint N);


float3 Hamersley_cosine(uint i, uint N);

float3 F_Schlick(const float3 f0, const float f90, const float u);

float disneyDiffuse(float  NdotV , float  NdotL , float  LdotH ,float linearRoughness);

#endif
