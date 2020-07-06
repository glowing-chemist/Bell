#ifndef RT_SAMPLERS_HPP
#define RT_SAMPLERS_HPP

#include "Engine/GeomUtils.h"

struct Sample
{
    float3 L;
    float P;
};

class DiffuseSampler
{
public:
    DiffuseSampler(const uint32_t maxSamples) :
    mMaxSamples(maxSamples),
    mGeneratedSamples(0) {}

    Sample generateSample(const float3 &normal);

private:

    uint32_t mMaxSamples;
    uint32_t mGeneratedSamples;
};

#endif
