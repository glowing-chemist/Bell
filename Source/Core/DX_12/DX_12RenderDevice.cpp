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