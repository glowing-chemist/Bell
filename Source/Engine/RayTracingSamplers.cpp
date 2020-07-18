#include "Engine/RayTracingSamplers.hpp"
#include "Engine/PBR.hpp"
#include "Core/BellLogging.hpp"

Sample DiffuseSampler::generateSample(const float3& normal)
{
    BELL_ASSERT(mGeneratedSamples < mMaxSamples, "Adjust max samples")
    const float2 xi = Hammersley(mGeneratedSamples++, mMaxSamples);
    float3 L;
    float NdotL;
    importanceSampleCosDir(xi, normal, L, NdotL);

    return {L, 1.0f};
}


Sample SpecularSampler::generateSample(const float3& N, const float3& V, const float R)
{
    BELL_ASSERT(mGeneratedSamples < mMaxSamples, "Adjust max samples")
    float2 Xi = Hammersley(mGeneratedSamples++, mMaxSamples);
    // Sample microfacet direction
    float3 GGX_H = ImportanceSampleGGX(Xi, R, N);

    Sample samp{};
    // Get the light direction
    samp.L = glm::reflect(-V, GGX_H);
    const float NdotL = glm::dot(N, samp.L);
    //BELL_ASSERT(NdotL > 0.0f, "too small")
    samp.P = std::max(NdotL, 0.0f);

    return samp;
}
