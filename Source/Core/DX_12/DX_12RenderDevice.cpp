#include "DX_12RenderDevice.hpp"
#include "DX_12SwapChain.hpp"
#include "Core/BellLogging.hpp"

#include <D3d12.h>

DX_12RenderDevice::DX_12RenderDevice(ID3D12Device* dev, IDXGIAdapter* adapter, IDXGIFactory* deviceFactory, GLFWwindow* window) :
	mDevice(static_cast<ID3D12Device6*>(dev)),
	mAdapter(static_cast<IDXGIAdapter3*>(adapter)),
	mWindow(window)
{
	D3D12MA::ALLOCATOR_DESC desc{};
	desc.Flags = D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;
	desc.pDevice = mDevice;
	desc.PreferredBlockSize = 0;
	desc.pAllocationCallbacks = nullptr;
	desc.pAdapter = adapter;

	HRESULT result = D3D12MA::CreateAllocator(&desc, &mMemoryManager);
	BELL_ASSERT(result == S_OK, "Failed to create memory manager");

	// get the queues.
	{
		D3D12_COMMAND_QUEUE_DESC graphicsQueueDesc{};
		graphicsQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
		graphicsQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		graphicsQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		graphicsQueueDesc.NodeMask = 0;
		
		HRESULT result = mDevice->CreateCommandQueue(&graphicsQueueDesc, IID_PPV_ARGS(&mGraphicsQueue));
		BELL_ASSERT(result == S_OK, "Failed to create graphics queue")
		

		D3D12_COMMAND_QUEUE_DESC computeQueueDesc {};
		computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
		computeQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
		computeQueueDesc.NodeMask = 0;

		result = mDevice->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&mComputeQueue));
		BELL_ASSERT(result == S_OK, "Failed to create compute queue")
	}

	mSwapChain = static_cast<SwapChainBase*>(new DX_12SwapChain(this, deviceFactory, window));

	for (uint32_t i = 0; i < mSwapChain->getNumberOfSwapChainImages(); ++i)
	{
		// Create frame complete fences.
		ID3D12Fence* fence;
		HRESULT result = mDevice->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		BELL_ASSERT(result == S_OK, "Failed to create fence")
		mFrameComplete.push_back(fence);
	}
}


DX_12RenderDevice::~DX_12RenderDevice()
{
	mMemoryManager->Release();
	mMemoryManager = nullptr;

	mGraphicsQueue->Release();
	mComputeQueue->Release();

	for (auto* fence : mFrameComplete)
		fence->Release();
}


CommandContextBase* DX_12RenderDevice::getCommandContext(const uint32_t index)
{
	BELL_TRAP;
	return nullptr;
}


void DX_12RenderDevice::startFrame()
{
	ID3D12Fence* currentFence = mFrameComplete[mCurrentFrameIndex];
	mGraphicsQueue->Wait(currentFence, 1);
	currentFence->Signal(0); // Reset.
	mMemoryManager->SetCurrentFrameIndex(mCurrentSubmission);
}


void DX_12RenderDevice::endFrame()
{
	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mSwapChain->getNumberOfSwapChainImages();
	mCurrentSubmission++;
}


void DX_12RenderDevice::destroyImage(ImageBase& image)
{
}


void DX_12RenderDevice::destroyImageView(ImageViewBase& view)
{

}


void DX_12RenderDevice::destroyBuffer(BufferBase& buffer)
{

}


void DX_12RenderDevice::destroyShaderResourceSet(const ShaderResourceSetBase& set)
{

}


void DX_12RenderDevice::setDebugName(const std::string& name, const uint64_t objPtr, const uint64_t)
{
	BELL_ASSERT(name.size() < 32, "Make buffer larger or name smaller")
	wchar_t buf[32];
	swprintf(buf, L"%S", name.c_str());

	ID3D12Object* object = reinterpret_cast<ID3D12Object*>(objPtr);
	object->SetName(buf);
}


void DX_12RenderDevice::flushWait() const
{
}


void DX_12RenderDevice::invalidatePipelines()
{

}

void DX_12RenderDevice::submitContext(CommandContextBase*, const bool finalSubmission)
{

}


void DX_12RenderDevice::swap()
{
	mSwapChain->present(QueueType::Graphics);
}


size_t DX_12RenderDevice::getMinStorageBufferAlignment() const
{
	BELL_TRAP; // TODO Look in to this.
	return 16;
}


const std::vector<uint64_t>& DX_12RenderDevice::getAvailableTimestamps() const
{
	return mTimeStamps;
}


float                       DX_12RenderDevice::getTimeStampPeriod() const
{
	BELL_TRAP;
	return 1.0f;
}


ID3D12DescriptorHeap* DX_12RenderDevice::createShaderInputDescriptorHeap(const uint32_t numDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = numDescriptors;

	ID3D12DescriptorHeap* heap;
	HRESULT result = mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	BELL_ASSERT(result == S_OK, "Failed to create descriptor heap")

	return heap;
}


void DX_12RenderDevice::createResource(	const D3D12_RESOURCE_DESC& desc, 
										const D3D12MA::ALLOCATION_DESC& allocDesc, 
										const D3D12_RESOURCE_STATES initialResourceState, 
										ID3D12Resource** outResource, 
										D3D12MA::Allocation** outResourceMemory, 
										D3D12_CLEAR_VALUE* clearValue)
{
	BELL_ASSERT(outResource, "Need to supply a non null resource ptr");
	BELL_ASSERT(outResourceMemory, "Need to supply a non null resource memory ptr");

	HRESULT result = mMemoryManager->CreateResource(&allocDesc, &desc, initialResourceState,
													clearValue, outResourceMemory, IID_PPV_ARGS(outResource));
	BELL_ASSERT(result == S_OK, "Failed to create resource");
}


D3D12MA::ALLOCATION_DESC DX_12RenderDevice::getResourceAllocationDescription(const ImageUsage usage)
{
	D3D12MA::ALLOCATION_DESC desc{};
	desc.CustomPool = nullptr;
	desc.HeapType = usage & ImageUsage::TransferSrc ? D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	desc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
	desc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

	return desc;
}


D3D12MA::ALLOCATION_DESC DX_12RenderDevice::getResourceAllocationDescription(const BufferUsage usage)
{
	D3D12MA::ALLOCATION_DESC desc{};
	desc.CustomPool = nullptr;
	desc.HeapType = (usage & BufferUsage::TransferSrc || usage & BufferUsage::Uniform) ? D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	desc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
	desc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

	return desc;
}
