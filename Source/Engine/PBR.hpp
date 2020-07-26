#ifndef PBR_HPP
#define PBR_HPP

#include "Engine/GeomUtils.h"

void  importanceSampleCosDir(const float2& u, const float3& N, float3& L, float& NdotL);

float3 ImportanceSampleGGX(const float2 &Xi, const float roughness, const float3 &N);

float RadicalInverse_VdC(uint32_t bits);


float3 hemisphereSample_uniform(float u, float v);


float3 hemisphereSample_cos(float u, float v);


float2 Hammersley(uint32_t i, uint32_t N);


float3 Hamersley_uniform(uint32_t i, uint32_t N);


float3 Hamersley_cosine(uint32_t i, uint32_t N);

float3 F_Schlick(const float3 f0, const float f90, const float u);

float disneyDiffuse(float  NdotV , float  NdotL , float  LdotH ,float linearRoughness);

float specular_GGX(const float3& N, const float3& V, const float3& L, const float roughness, const float3 &F0);

#endif
