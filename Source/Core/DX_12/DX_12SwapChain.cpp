#include "DX_12Swapchain.hpp"
#include "DX_12RenderDevice.hpp"

#include "Core/BellLogging.hpp"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


DX_12SwapChain::DX_12SwapChain(RenderDevice* dev, IDXGIFactory* factory, GLFWwindow* window)
: SwapChainBase{dev, window}
{
	DX_12RenderDevice* device = static_cast<DX_12RenderDevice*>(getDevice());
	
	
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	DXGI_MODE_DESC modeDesc{};
	modeDesc.Width = width;
	modeDesc.Height = height;
	modeDesc.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	modeDesc.RefreshRate = { 60, 1 };
	modeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	modeDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;

	DXGI_SWAP_CHAIN_DESC swapchainDesc{};
	swapchainDesc.BufferDesc = modeDesc;
	swapchainDesc.SampleDesc = { 1, 1 }; // Just a single sample please.
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.BufferCount = 3;
	swapchainDesc.OutputWindow = glfwGetWin32Window(window);
	swapchainDesc.Windowed = true;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	HRESULT result = factory->CreateSwapChain(device->getGraphicsQueue(), &swapchainDesc, &mSwapChain);
	BELL_ASSERT(result == S_OK, "Failed to create swapchain")
}


DX_12SwapChain::~DX_12SwapChain()
{

}

uint32_t DX_12SwapChain::getNextImageIndex()
{
	mCurrentImageIndex = (mCurrentImageIndex + 1) % 3;
	return mCurrentImageIndex;
}


void DX_12SwapChain::present(const QueueType)
{
	mSwapChain->Present(0, DXGI_PRESENT_DO_NOT_SEQUENCE);
}
