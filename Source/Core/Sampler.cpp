#include "Core/Sampler.hpp"

namespace std
{
    size_t hash<Sampler>::operator()(const Sampler& sampler) const noexcept
    {
        std::hash<uint8_t> hasher{};

        return hasher(sampler.mSamples) | hasher(static_cast<uint8_t>(sampler.mType));
    }
}


bool operator==(const Sampler& lhs, const Sampler& rhs)
{
    std::hash<Sampler> hasher{};

    return hasher(lhs) == hasher(rhs);
}

