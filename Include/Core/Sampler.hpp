#ifndef SAMPLER_HPP
#define SAMPLER_HPP

#include <array>
#include <cstdint>
#include <functional>


enum class SamplerType
{
    Linear,
    Point
};

enum class AddressMode
{
    Clamp,
    Mirror,
    Repeat
};


class Sampler
{
    friend std::hash<Sampler>;

public:
    Sampler(const SamplerType type) :
        mType{type},
        mSamples{1},
        mU{AddressMode::Repeat},
        mV{AddressMode::Repeat},
        mW{AddressMode::Repeat} {}

    void setNumberOfSamples(const uint8_t samples)
        { mSamples = samples; }

    uint8_t getNumberOfSamples() const
        { return mSamples; }

    void setAddressModeU(const AddressMode mode)
        { mU = mode; }

    void setAddressModeV(const AddressMode mode)
        { mV = mode; }

    void setAddressModeW(const AddressMode mode)
        { mW = mode; }

    std::array<AddressMode, 3> getAddressModes() const
        { return {mU, mV, mW}; }

    SamplerType getSamplerType() const
        { return mType; }

private:

    SamplerType mType;
    uint8_t mSamples;

    AddressMode mU;
    AddressMode mV;
    AddressMode mW;
};


namespace std
{
    template<>
    struct hash<Sampler>
    {
        size_t operator()(const Sampler&) const noexcept;
    };
}

bool operator==(const Sampler&, const Sampler&);

#endif
