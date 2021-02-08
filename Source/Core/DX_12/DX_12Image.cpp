#include "DX_12Image.hpp"
#include "DX_12RenderDevice.hpp"
#include "Core/ConversionUtils.hpp"

#include <algorithm>

DX_12Image::DX_12Image( RenderDevice* device,
                        const Format format,
                        const ImageUsage usage,
                        const uint32_t x,
                        const uint32_t y,
                        const uint32_t z,
                        const uint32_t mips,
                        const uint32_t levels,
                        const uint32_t samples,
                        const std::string& name) :
    ImageBase(device, format, usage, x, y, z, mips, levels, samples, name),
    mIsOwned(true)
{
    D3D12_RESOURCE_DIMENSION type;
    if (x != 0 && y == 0 && z == 0) type = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    if (x != 0 && y != 0 && z == 1) type = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    if (x != 0 && y != 0 && z > 1) type = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D;

    D3D12_RESOURCE_DESC imageDesc{};
    imageDesc.Width = x;
    imageDesc.Height = y;
    imageDesc.DepthOrArraySize = std::max(z, levels);
    imageDesc.Dimension = type;
    imageDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
    imageDesc.Format = getDX12ImageFormat(format);
    imageDesc.MipLevels = mips;
    imageDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    imageDesc.Flags = getDX12ImageUsage(usage);

    DXGI_SAMPLE_DESC sampleDesc{};
    sampleDesc.Count = samples;
    sampleDesc.Quality = 0;
    imageDesc.SampleDesc = sampleDesc;

    DX_12RenderDevice* dev = static_cast<DX_12RenderDevice*>(getDevice());
    const D3D12MA::ALLOCATION_DESC allocDesc = dev->getResourceAllocationDescription(usage);

    dev->createResource(imageDesc, allocDesc, D3D12_RESOURCE_STATE_COMMON , &mImage, &mImageMemory);
}


DX_12Image::DX_12Image(RenderDevice* device,
    ID3D12Resource* resource,
    const Format format,
    const ImageUsage usage,
    const uint32_t x,
    const uint32_t y,
    const uint32_t z,
    const uint32_t mips,
    const uint32_t levels,
    const uint32_t samples,
    const std::string& name) :
    ImageBase(device, format, usage, x, y, z, mips, levels, samples, name)
{
    mImage = resource;
    mImageMemory = nullptr;
    mIsOwned = false;
}


DX_12Image::~DX_12Image()
{
    if(mIsOwned)
        getDevice()->destroyImage(*this);
}


void DX_12Image::swap(ImageBase& other)
{
    ImageBase::swap(other);

    DX_12Image& DXImage = static_cast<DX_12Image&>(other);

    ID3D12Resource* tmpImage = mImage;
    D3D12MA::Allocation* tmpMemory = DXImage.mImageMemory;

    mImage = DXImage.mImage;
    mImageMemory = DXImage.mImageMemory;

    DXImage.mImage = tmpImage;
    DXImage.mImageMemory = tmpMemory;
}


void DX_12Image::setContents(   const void* data,
                                const uint32_t xsize,
                                const uint32_t ysize,
                                const uint32_t zsize,
                                const uint32_t level,
                                const uint32_t lod,
                                const int32_t offsetx,
                                const int32_t offsety,
                                const int32_t offsetz)
{
    BELL_TRAP;
}


void DX_12Image::clear(const float4&)
{
    BELL_TRAP;
}


void DX_12Image::generateMips()
{
    BELL_TRAP;
}