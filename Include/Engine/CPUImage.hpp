#ifndef CPU_IMAGE_HPP
#define CPU_IMAGE_HPP

#include "PassTypes.hpp"
#include "GeomUtils.h"

#include "Core/Image.hpp"

class CPUImage
{
public:
    CPUImage(std::vector<unsigned char>&& data, const ImageExtent& extent, const Format format) :
    mData(data),
    mExtent(extent),
    mFormat(format) {}

    ~CPUImage();

    float sample(const float2& uv) const;
    float2 sample2(const float2& uv) const;
    float3 sample3(const float2& uv) const;
    float4 sample4(const float2& uv) const;

    float sampleCube(const float3& uv) const;
    float2 sampleCube2(const float3& uv) const;
    float3 sampleCube3(const float3& uv) const;
    float4 sampleCube4(const float3& uv) const;

private:

    std::vector<unsigned char> mData;
    ImageExtent mExtent;
    Format mFormat;

};

#endif
