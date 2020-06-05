#ifndef DX_12_SWAPCHINA_HPP
#define DX_12_SWAPCHINA_HPP

#include "Core/Swapchain.hpp"
#include <d3d12.h>
#include <dxgi.h>

class DX_12SwapChain : public SwapChainBase
{
public:
	DX_12SwapChain(RenderDevice*, IDXGIFactory*, GLFWwindow*);
	~DX_12SwapChain();

	virtual uint32_t getNextImageIndex() override final;
	virtual void present(const QueueType queueIndex) override final;

	virtual uint32_t getNumberOfSwapChainImages() const override
	{
		return 3; // For now just always use 3.
	}

	virtual ImageView& getImageView(const size_t index) override final;

	virtual const ImageView& getImageView(const size_t index) const override final;

	virtual Image& getImage(const size_t index) override final;

	virtual const Image& getImage(const size_t index) const override final;

private:

	IDXGISwapChain* mSwapChain;

	ID3D12Resource* mCurrentResource;

	Image* mCurrentImage;
	ImageView* mCurrentImageView;
};

#endif