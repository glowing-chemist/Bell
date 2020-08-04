#include "Core/DX_12/DX_12Buffer.hpp"
#include "Core/DX_12/DX_12RenderDevice.hpp"


DX_12Buffer::DX_12Buffer(RenderDevice* dev,
    BufferUsage usage,
    const uint32_t size,
    const uint32_t stride,
    const std::string& name) :
    BufferBase(dev, usage, size, stride, name),
    mMappedPtr{nullptr}
{
    D3D12_RESOURCE_DESC bufferDesc{};
    bufferDesc.Width = size;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    if(usage & BufferUsage::DataBuffer)
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    DXGI_SAMPLE_DESC sampleDesc{};
    sampleDesc.Count = 1;
    sampleDesc.Quality = 0;
    bufferDesc.SampleDesc = sampleDesc;

    DX_12RenderDevice* DXdev = static_cast<DX_12RenderDevice*>(getDevice());
    const D3D12MA::ALLOCATION_DESC allocDesc = DXdev->getResourceAllocationDescription(usage);

    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        initialState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (usage & BufferUsage::Index || usage & BufferUsage::Vertex || usage & BufferUsage::Uniform)
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    else if (usage & BufferUsage::TransferDest)
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
    else if (usage & BufferUsage::TransferSrc)
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

    DXdev->createResource(bufferDesc, allocDesc, initialState, &mBuffer, &mBufferMemory);

    if(usage & BufferUsage::TransferSrc || usage & BufferUsage::Uniform)
        mBuffer->Map(0, nullptr, &mMappedPtr);
}


DX_12Buffer::~DX_12Buffer()
{

}

void DX_12Buffer::swap(BufferBase&)
{

}

bool DX_12Buffer::resize(const uint32_t newSize, const bool preserContents)
{
    return false;
}


void DX_12Buffer::setContents(const void* data,
    const uint32_t size,
    const uint32_t offset)
{

}

// Repeat the data in the range (start + offset, start + offset + size]
void DX_12Buffer::setContents(const int data,
    const uint32_t size,
    const uint32_t offset)
{

}


void* DX_12Buffer::map(MapInfo& mapInfo)
{
    return mMappedPtr;
}


void    DX_12Buffer::unmap()
{
    // No-Op
}
