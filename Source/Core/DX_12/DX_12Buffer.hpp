#ifndef DX_12_BUFFER_HPP
#define DX_12_BUFFER_HPP

#include <d3d12.h>
#include "D3D12MemAlloc.h"

#include "Core/Buffer.hpp"

class DX_12Buffer : public BufferBase
{
public:
    DX_12Buffer(RenderDevice* dev,
        BufferUsage usage,
        const uint32_t size,
        const uint32_t stride,
        const std::string & = "");

    ~DX_12Buffer();

    virtual void swap(BufferBase&) override final;

    virtual bool resize(const uint32_t newSize, const bool preserContents) override final;

    virtual void setContents(const void* data,
        const uint32_t size,
        const uint32_t offset = 0) override final;

    // Repeat the data in the range (start + offset, start + offset + size]
    virtual void setContents(const int data,
        const uint32_t size,
        const uint32_t offset = 0) override final;


    virtual void* map(MapInfo& mapInfo) override final;
    virtual void    unmap() override final;

    ID3D12Resource* getResourceHandle()
    {
        return mBuffer;
    }

    const ID3D12Resource* getResourceHandle() const
    {
        return mBuffer;
    }

private:

    ID3D12Resource* mBuffer;
    D3D12MA::Allocation* mBufferMemory;
    void* mMappedPtr; // persistently mapped.

};

#endif