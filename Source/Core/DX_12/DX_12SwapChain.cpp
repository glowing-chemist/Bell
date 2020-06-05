#include "DX_12Swapchain.hpp"
#include "DX_12RenderDevice.hpp"
#include "DX_12Image.hpp"
#include "DX_12ImageView.hpp"

#include "Core/BellLogging.hpp"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


DX_12SwapChain::DX_12SwapChain(RenderDevice* dev, IDXGIFactory* factory, GLFWwindow* window)
: SwapChainBase{dev, window}
{
	DX_12RenderDevice* device = static_cast<DX_12RenderDevice*>(getDevice());
	
	
	const ImageExtent extent = chooseSwapExtent(window);

	DXGI_MODE_DESC modeDesc{};
	modeDesc.Width = extent.width;
	modeDesc.Height = extent.height;
	modeDesc.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
	modeDesc.RefreshRate = { 60, 1 };
	modeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	modeDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;
	
	DXGI_SWAP_CHAIN_DESC swapchainDesc{};
	swapchainDesc.BufferDesc = modeDesc;
	swapchainDesc.SampleDesc = { 1, 0 }; // Just a single sample please.
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.BufferCount = 3;
	swapchainDesc.OutputWindow = glfwGetWin32Window(window);
	swapchainDesc.Windowed = true;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	HRESULT result = factory->CreateSwapChain(device->getGraphicsQueue(), &swapchainDesc, &mSwapChain);
	BELL_ASSERT(result == S_OK, "Failed to create swapchain")

	mSwapChain->GetBuffer(mCurrentImageIndex, IID_PPV_ARGS(&mCurrentResource));
	mCurrentImage = new Image(new DX_12Image(getDevice(), mCurrentResource, Format::BGRA8UNorm, ImageUsage::ColourAttachment, extent.width, extent.height, 1, 1, 1, 1, "Swapchain"));
	mCurrentImageView = new ImageView(*mCurrentImage, ImageViewType::Colour);
}


DX_12SwapChain::~DX_12SwapChain()
{
	mSwapChain->Release();
}


uint32_t DX_12SwapChain::getNextImageIndex()
{
	mCurrentImageIndex = (mCurrentImageIndex + 1) % 3;

	mSwapChain->GetBuffer(mCurrentImageIndex, IID_PPV_ARGS(&mCurrentResource));

	const ImageExtent extent = (*mCurrentImage)->getExtent(0, 0);
	delete mCurrentImage;
	delete mCurrentImageView;
	mCurrentImage = new Image(new DX_12Image(getDevice(), mCurrentResource, Format::BGRA8UNorm, ImageUsage::ColourAttachment, extent.width, extent.height, 1, 1, 1, 1, "Swapchain"));
	mCurrentImageView = new ImageView(*mCurrentImage, ImageViewType::Colour);

	return mCurrentImageIndex;
}


void DX_12SwapChain::present(const QueueType)
{
	mSwapChain->Present(0, DXGI_PRESENT_DO_NOT_SEQUENCE);
}


ImageView& DX_12SwapChain::getImageView(const size_t index)
{
	BELL_ASSERT(index == mCurrentImageIndex, "Not possible on DX12 backend")

	return *mCurrentImageView;
}


const ImageView& DX_12SwapChain::getImageView(const size_t index) const
{
	BELL_ASSERT(index == mCurrentImageIndex, "Not possible on DX12 backend")

	return *mCurrentImageView;
}


Image& DX_12SwapChain::getImage(const size_t index)
{
	BELL_ASSERT(index == mCurrentImageIndex, "Not possible on DX12 backend")

	return *mCurrentImage;
}


const Image& DX_12SwapChain::getImage(const size_t index) const
{
	BELL_ASSERT(index == mCurrentImageIndex, "Not possible on DX12 backend")

	return *mCurrentImage;
}
