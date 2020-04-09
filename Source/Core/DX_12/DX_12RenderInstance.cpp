#include "DX_12RenderInstance.hpp"
#include "DX_12RenderDevice.hpp"
#include "Core/BellLogging.hpp"

#include <D3d12.h>


DX_12RenderInstance::DX_12RenderInstance(GLFWwindow* window) :
	RenderInstance(window),
	mDebugHandle{nullptr},
	mFactory{nullptr},
	mAdapter{nullptr},
	mDevice{nullptr}
{
	uint32_t factoryFlags = 0;
#ifndef NDEBUG
	factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	HRESULT result = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&mFactory));
	BELL_ASSERT(result == S_OK, "Failed to create instace factory");

	enableDebugCallback();
}


DX_12RenderInstance::~DX_12RenderInstance()
{
	mDevice->Release();
	mAdapter->Release();
	mFactory->Release();
	mDebugHandle->Release();
}


RenderDevice* DX_12RenderInstance::createRenderDevice(const int DeviceFeatureFlags)
{
	const bool geometryWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Geometry;
	const bool tessWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Tessalation;
	const bool discreteWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Discrete;
	const bool computeWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Compute;
	const bool subgroupWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Subgroup;

	// Find the adapter with the larget amout of video memory
	IDXGIAdapter1* testAdapter;
	uint32_t maxVideomemory = 0;
	LUID adapterLUID{0, 0};
	for (uint32_t i = 0; mFactory->EnumAdapters1(i, &testAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 desc1;
		testAdapter->GetDesc1(&desc1);

		BELL_LOG_ARGS("Device Found %S", desc1.Description);

		if (desc1.DedicatedVideoMemory > maxVideomemory)
		{
			maxVideomemory = desc1.DedicatedVideoMemory;
			adapterLUID = desc1.AdapterLuid;
		}
	}

	IDXGIAdapter3* chosenAdapter = nullptr;
	mFactory->EnumAdapterByLuid(adapterLUID, IID_PPV_ARGS(&chosenAdapter));
	BELL_ASSERT(chosenAdapter, "Unable to fetch adapter");
	mAdapter = chosenAdapter;

	ID3D12Device6* device = nullptr;
	HRESULT result = D3D12CreateDevice(chosenAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
	BELL_ASSERT(device, "Unable to create device");

	if (subgroupWanted)
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS1 subgroupSupportInfo{};
		HRESULT result = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1 , &subgroupSupportInfo, sizeof(subgroupSupportInfo));
		BELL_ASSERT(result == S_OK, "Unable to fetch subgroup info");

		BELL_ASSERT(subgroupSupportInfo.WaveOps, "Subgroup support requested but not supported by device");
	}

	return new DX_12RenderDevice(device, mAdapter, mWindow);
}


void DX_12RenderInstance::enableDebugCallback()
{
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&mDebugHandle));
	BELL_ASSERT(result == S_OK, "Failed to create debug Interface");

	mDebugHandle->EnableDebugLayer();
}