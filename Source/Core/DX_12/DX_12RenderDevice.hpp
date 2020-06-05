#ifndef DX_12_RENDER_DEVICE_HPP
#define DX_12_RENDER_DEVICE_HPP

#include "Core/RenderDevice.hpp"
#include "D3D12MemAlloc.h"
#include <d3d12.h>
#include <dxgi1_4.h>

class DX_12RenderDevice : public RenderDevice
{
public:
	DX_12RenderDevice(ID3D12Device*, IDXGIAdapter* adapter, IDXGIFactory* deviceFactory, GLFWwindow*);
	~DX_12RenderDevice();

	virtual CommandContextBase*		   getCommandContext(const uint32_t index) override;

	virtual void                       startFrame() override;
	virtual void                       endFrame() override;

	virtual void                       destroyImage(ImageBase& image) override;
	virtual void                       destroyImageView(ImageViewBase& view) override;

	virtual void                       destroyBuffer(BufferBase& buffer) override;

	virtual void					   destroyShaderResourceSet(const ShaderResourceSetBase& set) override;

	virtual void					   setDebugName(const std::string&, const uint64_t, const uint64_t objectType) override;

	virtual void                       flushWait() const override;
    virtual void                       invalidatePipelines() override;

	virtual void					   submitContext(CommandContextBase*, const bool finalSubmission = false) override;
	virtual void					   swap() override;

	virtual size_t					   getMinStorageBufferAlignment() const override;

	virtual const std::vector<uint64_t>& getAvailableTimestamps() const override;
	virtual float                      getTimeStampPeriod() const override;

	void							   createImage(	const D3D12_RESOURCE_DESC& desc, 
													const D3D12MA::ALLOCATION_DESC& allocDesc, 
													ID3D12Resource** outImage, 
													D3D12MA::Allocation** outImageMemory,
													D3D12_CLEAR_VALUE* clearValue = nullptr);

	D3D12MA::ALLOCATION_DESC		   getResourceAllocationDescription(const ImageUsage);
	D3D12MA::ALLOCATION_DESC		   getResourceAllocationDescription(const BufferUsage);

	ID3D12CommandQueue* getGraphicsQueue()
	{
		return mGraphicsQueue;
	}

private:

	ID3D12Device6* mDevice;
	IDXGIAdapter3* mAdapter;
	GLFWwindow* mWindow;

	D3D12MA::Allocator* mMemoryManager;

	ID3D12CommandQueue* mGraphicsQueue;
	ID3D12CommandQueue* mComputeQueue;

	std::vector<ID3D12Fence*> mFrameComplete;

	std::vector<uint64_t> mTimeStamps;
};

#endif
