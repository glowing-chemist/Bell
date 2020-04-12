#include "DX_12RenderDevice.hpp"
#include "Core/BellLogging.hpp"


DX_12RenderDevice::DX_12RenderDevice(ID3D12Device* dev, IDXGIAdapter* adapter, GLFWwindow* window) :
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
}


DX_12RenderDevice::~DX_12RenderDevice()
{
	mMemoryManager->Release();
	mMemoryManager = nullptr;
}


void DX_12RenderDevice::generateFrameResources(RenderGraph&, const uint64_t)
{
}


void DX_12RenderDevice::startPass(const RenderTask&)
{

}


Executor* DX_12RenderDevice::getPassExecutor()
{
	return nullptr;
}


void DX_12RenderDevice::freePassExecutor(Executor*)
{

}


void DX_12RenderDevice::endPass()
{

}


void DX_12RenderDevice::execute(BarrierRecorder& recorder)
{

}


void DX_12RenderDevice::startFrame()
{

}


void DX_12RenderDevice::endFrame()
{

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


void DX_12RenderDevice::setDebugName(const std::string&, const uint64_t, const uint64_t objectType)
{

}


void DX_12RenderDevice::flushWait() const
{

}


void DX_12RenderDevice::submitFrame()
{

}


void DX_12RenderDevice::swap()
{

}


size_t DX_12RenderDevice::getMinStorageBufferAlignment() const
{
	return ~0ULL;
}


void DX_12RenderDevice::createImage(const D3D12_RESOURCE_DESC& desc, const D3D12MA::ALLOCATION_DESC& allocDesc, ID3D12Resource** outImage, D3D12MA::Allocation** outImageMemory)
{
	BELL_ASSERT(outImage, "Need to supply a non null image ptr");
	BELL_ASSERT(outImageMemory, "Need to supply a non null image memory ptr");

	D3D12_CLEAR_VALUE clearVal;
	if (desc.Format & DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT)
	{
		clearVal.DepthStencil.Depth = 0.0f;
		clearVal.DepthStencil.Stencil = 0;
	}
	else
	{
		clearVal.Color[0] = 0.0f;
		clearVal.Color[1] = 0.0f;
		clearVal.Color[2] = 0.0f;
		clearVal.Color[3] = 0.0f;
	}

	HRESULT result = mMemoryManager->CreateResource(&allocDesc, &desc, allocDesc.HeapType & D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD ? D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, 
													&clearVal, outImageMemory, IID_PPV_ARGS(outImage));
	BELL_ASSERT(result == S_OK, "Failed to create Image");
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
	desc.HeapType = usage & BufferUsage::TransferSrc ? D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	desc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
	desc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

	return desc;
}
