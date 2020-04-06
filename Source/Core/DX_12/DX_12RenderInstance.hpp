#ifndef DX_12_RENDER_INSTANCE_HPP
#define DX_12_RENDER_INSTANCE_HPP

#include "Core/RenderInstance.hpp"
#include <wrl.h>
#include <d3d12sdklayers.h>
#include <dxgi1_4.h>


class DX_12RenderInstance : public RenderInstance
{
public:
	DX_12RenderInstance(GLFWwindow*);
	~DX_12RenderInstance();

	virtual RenderDevice* createRenderDevice(const int DeviceFeatureFlags = 0) override final;

private:

	void enableDebugCallback();

	ID3D12Debug* mDebugHandle;

	IDXGIFactory4* mFactory;
	ID3D12Device6* mDevice;

};

#endif