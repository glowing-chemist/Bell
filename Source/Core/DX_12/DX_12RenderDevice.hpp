#ifndef DX_12_RENDER_DEVICE_HPP
#define DX_12_RENDER_DEVICE_HPP

#include "Core/RenderDevice.hpp"
#include "D3D12MemAlloc.h"
#include <d3d12.h>
#include <dxgi1_4.h>

class DX_12RenderDevice : public RenderDevice
{
public:
	DX_12RenderDevice(ID3D12Device*, IDXGIAdapter* adapter, GLFWwindow*);
	~DX_12RenderDevice();

	virtual void					   generateFrameResources(RenderGraph&) override;

	virtual void                       startPass(const RenderTask&) override;
	virtual Executor*				   getPassExecutor() override;
	virtual void					   freePassExecutor(Executor*) override;
	virtual void					   endPass() override;

	virtual void					   execute(BarrierRecorder& recorder) override;

	virtual void                       startFrame() override;
	virtual void                       endFrame() override;

	virtual void                       destroyImage(ImageBase& image) override;
	virtual void                       destroyImageView(ImageViewBase& view) override;

	virtual void                       destroyBuffer(BufferBase& buffer) override;

	virtual void					   destroyShaderResourceSet(const ShaderResourceSetBase& set) override;

	virtual void					   setDebugName(const std::string&, const uint64_t, const uint64_t objectType) override;

	virtual void                       flushWait() const override;

	virtual void					   submitFrame() override;
	virtual void					   swap() override;

	virtual size_t					   getMinStorageBufferAlignment() const override;

	void							   createImage(const D3D12_RESOURCE_DESC& desc, const D3D12MA::ALLOCATION_DESC& allocDesc, ID3D12Resource** outImage, D3D12MA::Allocation** outImageMemory);

	D3D12MA::ALLOCATION_DESC		   getResourceAllocationDescription(const ImageUsage);
	D3D12MA::ALLOCATION_DESC		   getResourceAllocationDescription(const BufferUsage);

private:

	ID3D12Device6* mDevice;
	IDXGIAdapter3* mAdapter;
	GLFWwindow* mWindow;

	D3D12MA::Allocator* mMemoryManager;

};

#endif