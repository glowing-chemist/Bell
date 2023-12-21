#ifndef DECSRIPTOR_ALLOCATOR_HPP
#define DECSRIPTOR_ALLOCATOR_HPP

#include <d3d12.h>
#include <cstdint>

enum class DescriptorVisibility
{
    CPU,
    GPU
};

class DescriptorAllocator
{
public:
    DescriptorAllocator(ID3D12Device* device,
                        ID3D12DescriptorHeap* heap,
                        const size_t size,
                        const D3D12_DESCRIPTOR_HEAP_TYPE type,
                        DescriptorVisibility);
    ~DescriptorAllocator();

    D3D12_CPU_DESCRIPTOR_HANDLE allocateCPUHandle();
    D3D12_GPU_DESCRIPTOR_HANDLE getCorrespondingCPUHandle(const D3D12_CPU_DESCRIPTOR_HANDLE&);

    void reset();

private:

    ID3D12Device* mDevice;
    ID3D12DescriptorHeap* mHeap;

    uint64_t mOffsetSize;

    D3D12_CPU_DESCRIPTOR_HANDLE mCPUDescriptorStart;
    uint64_t mCPUOffset;
    D3D12_GPU_DESCRIPTOR_HANDLE mGPUDescriptorStart;
};

#endif
