#ifndef DX_12_IMAGE_HPP
#define DX_12_IMAGE_HPP

#include "Core/Image.hpp"
#include "D3D12MemAlloc.h"

#include <d3d12.h>


class DX_12Image : public ImageBase
{
public:
	DX_12Image(RenderDevice*,
        const Format,
        const ImageUsage,
        const uint32_t x,
        const uint32_t y,
        const uint32_t z,
        const uint32_t mips = 1,
        const uint32_t levels = 1,
        const uint32_t samples = 1,
        const std::string& = "");

    DX_12Image(RenderDevice*,
        ID3D12Resource* resource,
        const Format,
        const ImageUsage,
        const uint32_t x,
        const uint32_t y,
        const uint32_t z,
        const uint32_t mips = 1,
        const uint32_t levels = 1,
        const uint32_t samples = 1,
        const std::string & = "");

	~DX_12Image();

    virtual void swap(ImageBase&) override final;

    virtual void setContents(const void* data,
        const uint32_t xsize,
        const uint32_t ysize,
        const uint32_t zsize,
        const uint32_t level = 0,
        const uint32_t lod = 0,
        const int32_t offsetx = 0,
        const int32_t offsety = 0,
        const int32_t offsetz = 0) override final;

    virtual void clear(const float4&) override final;

    virtual void generateMips() override final;

    ID3D12Resource* getResourceHandle()
    {
        return mImage;
    }

    const ID3D12Resource* getResourceHandle() const
    {
        return mImage;
    }

private:

    ID3D12Resource* mImage;
    D3D12MA::Allocation* mImageMemory;

    bool mIsOwned;
};

#endif