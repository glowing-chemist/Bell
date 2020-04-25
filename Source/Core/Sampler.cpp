#include "Core/Sampler.hpp"

namespace std
{
    size_t hash<Sampler>::operator()(const Sampler& sampler) const noexcept
    {
        size_t hash = sampler.mSamples;
        hash |= static_cast<size_t>(sampler.mType) << 4;
        hash |= static_cast<size_t>(sampler.mU) << 6;
        hash |= static_cast<size_t>(sampler.mV) << 9;
        hash |= static_cast<size_t>(sampler.mW) << 12;

        return hash;
    }
}


bool operator==(const Sampler& lhs, const Sampler& rhs)
{
    std::hash<Sampler> hasher{};

    return hasher(lhs) == hasher(rhs);
}

