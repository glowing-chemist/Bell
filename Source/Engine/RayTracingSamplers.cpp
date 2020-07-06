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
