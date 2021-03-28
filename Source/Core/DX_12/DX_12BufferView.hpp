#ifndef DX_12_BUFFER_VIEW_HPP
#define DX_12_BUFFER_VIEW_HPP

#include "Core/BufferView.hpp"

#include <d3d12.h>

class DX_12BufferView : public BufferViewBase
{
public:
    DX_12BufferView(Buffer&, const uint64_t offset = 0, const uint64_t size = ~0ULL);
    ~DX_12BufferView();

    ID3D12Resource* getBuffer()
    {
        return mResource;
    }

    D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(const size_t offset) const;

    D3D12_INDEX_BUFFER_VIEW getIndexBufferView(const size_t offset) const;

    D3D12_CPU_DESCRIPTOR_HANDLE getUAV() const
    {
        return mUAV;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE getSRV() const
    {
        return mSRV;
    }


private:

    D3D12_CPU_DESCRIPTOR_HANDLE mUAV;
    D3D12_CPU_DESCRIPTOR_HANDLE  mSRV;

    ID3D12Resource* mResource;
};

#endif