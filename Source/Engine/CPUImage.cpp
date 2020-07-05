#include "Engine/CPUImage.hpp"
#include "Core/ConversionUtils.hpp"


CPUImage::~CPUImage()
{

}

float CPUImage::sample(const float2& uv) const
{

}


float2 CPUImage::sample2(const float2& uv) const
{

}


float3 CPUImage::sample3(const float2& uv) const
{

}


float4 CPUImage::sample4(const float2& uv) const
{
    const uint32_t x = uint32_t(uv.x * mExtent.width) % mExtent.width;
    const uint32_t y = uint32_t(uv.y * mExtent.height) % mExtent.height;

    const uint32_t pixelSize = getPixelSize(mFormat);

    const unsigned char* data = mData.data();
    data += (pixelSize * y * mExtent.width) + (pixelSize * x);

    if(pixelSize == 16)
    {
        const float* pix = reinterpret_cast<const float*>(data);
        float4 c;
        c.x = pix[0];
        c.y = pix[1];
        c.z = pix[2];
        c.w = pix[3];

        return c;
    }
    else if(pixelSize == 4)
    {
        const uint8_t* pix = reinterpret_cast<const uint8_t*>(data);
        float4 c;
        c.x = pix[0] / 255.0f;
        c.y = pix[1] / 255.0f;
        c.z = pix[2] / 255.0f;
        c.w = pix[3] / 255.0f;

        return c;
    }

    BELL_TRAP;
    return float4{0.0f, 0.0f, 0.0f, 0.0f};
}


float CPUImage::sampleCube(const float3&) const
{
    BELL_TRAP;
    return 0.0f;
}


float2 CPUImage::sampleCube2(const float3&) const
{
    BELL_TRAP;
    return float2{};
}


float3 CPUImage::sampleCube3(const float3&) const
{
    BELL_TRAP;
    return float3{};
}


float4 CPUImage::sampleCube4(const float3&) const
{
    BELL_TRAP;
    return float4{};
}
