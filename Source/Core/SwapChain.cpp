#include "SwapChain.hpp"
#include "RenderInstance.hpp"
#include "RenderDevice.hpp"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <limits>

vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
{
    if(formats.size() == 1 && formats[0].format == vk::Format::eUndefined) { // indicates any format can be used, so pick the best
        vk::SurfaceFormatKHR form;
        form.format = vk::Format::eB8G8R8A8Unorm;
        form.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        return form;
    }
    return formats[0]; // if not pick the first format
}

uint32_t SwapChain::getSwapChainImageWidth() const
{
    return mSwapChainExtent.width;
}


uint32_t SwapChain::getSwapChainImageHeight() const
{
    return mSwapChainExtent.height;
}


uint32_t SwapChain::getNumberOfSwapChainImages() const
{
    return mSwapChainImages.size();
}


SwapChainSupportDetails SwapChain::querySwapchainSupport(vk::PhysicalDevice dev, vk::SurfaceKHR surface)
{
    SwapChainSupportDetails details;

    details.capabilities = dev.getSurfaceCapabilitiesKHR(surface);
    details.formats = dev.getSurfaceFormatsKHR(surface);
    details.presentModes = dev.getSurfacePresentModesKHR(surface);

    return details;
}

vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes)
{
    for(auto& mode : presentModes) {
        if(mode == vk::PresentModeKHR::eMailbox) { // if availble return mailbox
            return mode;
        }
    }
    return vk::PresentModeKHR::eFifo; // else Efifo is garenteed to be present
}

vk::Extent2D SwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR&, GLFWwindow* window)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    return vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

SwapChain::SwapChain(RenderDevice* Device, vk::SurfaceKHR windowSurface, GLFWwindow* window) :
    DeviceChild{Device},
    mCurrentImageIndex{0},
	mSurface{windowSurface},
	mWindow{window}
{
	initialize();
}


SwapChain::~SwapChain()
{
    destroy();
}


void SwapChain::destroy()
{
    mSwapChainImages.clear();
    mImageViews.clear();

    if(mSwapChain != vk::SwapchainKHR{nullptr})
    {
        getDevice()->destroySwapchain(mSwapChain);
        mSwapChain = nullptr;
    }
}


vk::Format SwapChain::getSwapChainImageFormat() const
{
    return mSwapChainFormat;
}


void SwapChain::createSwapChainImageViews()
{
    for(size_t i = 0; i < mSwapChainImages.size(); i++)
    {
		ImageView imgeView(mSwapChainImages[i], ImageViewType::Colour);
        mImageViews.push_back(imgeView);
    }

    BELL_LOG_ARGS("created %zu image Views", mSwapChainImages.size())
}


uint32_t SwapChain::getNextImageIndex(vk::Semaphore& sem)
{
    getDevice()->aquireNextSwapchainImage(mSwapChain, std::numeric_limits<uint64_t>::max(), sem, mCurrentImageIndex);
	
	return mCurrentImageIndex;
}


void SwapChain::present(VkQueue presentQueue, VkSemaphore waitSemaphore)
{
	// needed for calling the C API (in order to handle errors correclty)
	VkSwapchainKHR tempSwapchain = mSwapChain;

	VkPresentInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.pSwapchains = &tempSwapchain;
	info.swapchainCount = 1;
    info.pWaitSemaphores = &waitSemaphore;
    info.waitSemaphoreCount = 1;
	info.pImageIndices = &mCurrentImageIndex;

	VkResult result = vkQueuePresentKHR(presentQueue, &info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		recreateSwapchain();
}


void SwapChain::recreateSwapchain()
{
	destroy();

	// Just hang tight until the window is un minimized.
	int width = 0, height = 0;
	glfwGetFramebufferSize(mWindow, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(mWindow, &width, &height);
		glfwWaitEvents();
	}

	initialize();
}


void SwapChain::initialize()
{
	SwapChainSupportDetails swapDetails = querySwapchainSupport(*getDevice()->getPhysicalDevice(), mSurface);
	vk::SurfaceFormatKHR swapFormat = chooseSurfaceFormat(swapDetails.formats);
	vk::PresentModeKHR presMode = choosePresentMode(swapDetails.presentModes);
	vk::Extent2D swapExtent = chooseSwapExtent(swapDetails.capabilities, mWindow);

	uint32_t images = 0;
	if (swapDetails.capabilities.maxImageCount == 0) { // some intel GPUs return max image count of 0 so work around this here
		images = swapDetails.capabilities.minImageCount + 1;
	}
	else {
		images = swapDetails.capabilities.minImageCount + 1 > swapDetails.capabilities.maxImageCount ? swapDetails.capabilities.maxImageCount : swapDetails.capabilities.minImageCount + 1;
	}

	vk::SwapchainCreateInfoKHR info{}; // fill out the format and presentation mode info
	info.setSurface(mSurface);
	info.setImageExtent(swapExtent);
	info.setPresentMode(presMode);
	info.setImageColorSpace(swapFormat.colorSpace);
	info.setImageFormat(swapFormat.format);
	info.setMinImageCount(images);
	info.setImageArrayLayers(1);
	info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
	info.setImageSharingMode(vk::SharingMode::eExclusive); // graphics and present are the same queue
	info.setPreTransform(swapDetails.capabilities.currentTransform);
	info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
	info.setPresentMode(presMode);
	info.setClipped(true);

	mSwapChain = getDevice()->createSwapchain(info);
	mSwapChainExtent = swapExtent;
	mSwapChainFormat = swapFormat.format;

	auto swapChainImages = getDevice()->getSwapchainImages(mSwapChain);
	for (vk::Image image : swapChainImages)
	{
		mSwapChainImages.emplace_back(getDevice(),
			image,
			getBellImageFormat(mSwapChainFormat),
			ImageUsage::ColourAttachment,
			std::max(swapExtent.width, 1u),
			std::max(swapExtent.height, 1u),
			1, 1, 1, 1, "SwapChain");
	}

	createSwapChainImageViews();
}
