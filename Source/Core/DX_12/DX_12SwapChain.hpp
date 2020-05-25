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

private:

	IDXGISwapChain* mSwapChain;

};

#endif