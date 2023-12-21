#include "DescriptorAllocator.hpp"

DescriptorAllocator::DescriptorAllocator(
        ID3D12Device* device,
        ID3D12DescriptorHeap* heap,
        const size_t size,
        const D3D12_DESCRIPTOR_HEAP_TYPE type,
        DescriptorVisibility visibility) :
        mDevice(device),
        mHeap(heap)
{
    mCPUDescriptorStart = mHeap->GetCPUDescriptorHandleForHeapStart();
    mCPUOffset = 0;
    if(visibility == DescriptorVisibility::GPU)
        mGPUDescriptorStart = mHeap->GetGPUDescriptorHandleForHeapStart();

    mOffsetSize = mDevice->GetDescriptorHandleIncrementSize(type);
}


DescriptorAllocator::~DescriptorAllocator()
{
    mHeap->Release();
}


D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::allocateCPUHandle()
{
    D3D12_CPU_DESCRIPTOR_HANDLE newDescriptor{mCPUDescriptorStart.ptr + mCPUOffset};

    mCPUOffset += mOffsetSize;

    return newDescriptor;
}


D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::getCorrespondingCPUHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& descriptor)
{
    const uint64_t diff = descriptor.ptr - mCPUDescriptorStart.ptr;

    return D3D12_GPU_DESCRIPTOR_HANDLE{mGPUDescriptorStart.ptr + diff};
}

void DescriptorAllocator::reset()
{
    mCPUOffset = 0;
}