#include "VulkanSwapChain.hpp"
#include "VulkanImage.hpp"
#include "VulkanRenderInstance.hpp"
#include "VulkanRenderDevice.hpp"
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


SwapChainSupportDetails VulkanSwapChain::querySwapchainSupport(vk::PhysicalDevice dev, vk::SurfaceKHR surface)
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


VulkanSwapChain::VulkanSwapChain(RenderDevice* Device, vk::SurfaceKHR windowSurface, GLFWwindow* window) :
    SwapChainBase{Device, window},
	mSurface{windowSurface}
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

	initialize();

	vk::SemaphoreCreateInfo semInfo{};
	for (uint32_t i = 0; i < getNumberOfSwapChainImages(); ++i)
	{
		mImageAquired.push_back(device->createSemaphore(semInfo));
		mImageRendered.push_back(device->createSemaphore(semInfo));
	}
}


VulkanSwapChain::~VulkanSwapChain()
{
    destroy();
}


void VulkanSwapChain::destroy()
{
    mSwapChainImages.clear();
    mImageViews.clear();

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

	for (auto& semaphore : mImageAquired)
	{
		device->destroySemaphore(semaphore);
	}

	for (auto& semaphore : mImageRendered)
	{
		device->destroySemaphore(semaphore);
	}

    if(mSwapChain != vk::SwapchainKHR{nullptr})
    {
        static_cast<VulkanRenderDevice*>(getDevice())->destroySwapchain(mSwapChain);
        mSwapChain = nullptr;
    }
}


void VulkanSwapChain::createSwapChainImageViews()
{
    for(size_t i = 0; i < mSwapChainImages.size(); i++)
    {
		ImageView imgeView(mSwapChainImages[i], ImageViewType::Colour);
        mImageViews.push_back(imgeView);
    }

    BELL_LOG_ARGS("created %zu image Views", mSwapChainImages.size())
}


uint32_t VulkanSwapChain::getNextImageIndex()
{
    static_cast<VulkanRenderDevice*>(getDevice())->aquireNextSwapchainImage(mSwapChain, std::numeric_limits<uint64_t>::max(), mImageAquired[mCurrentImageIndex], mCurrentImageIndex);
	
	return mCurrentImageIndex;
}


void VulkanSwapChain::present(const QueueType queue)
{
	// needed for calling the C API (in order to handle errors correclty)
	VkSwapchainKHR tempSwapchain = mSwapChain;
	VkSemaphore waitSemaphore = mImageRendered[mCurrentImageIndex];

	VkPresentInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.pSwapchains = &tempSwapchain;
	info.swapchainCount = 1;
    info.pWaitSemaphores = &waitSemaphore;
    info.waitSemaphoreCount = 1;
	info.pImageIndices = &mCurrentImageIndex;

	const VkQueue presentQueue = static_cast<VulkanRenderDevice*>(getDevice())->getQueue(queue);

	VkResult result = vkQueuePresentKHR(presentQueue, &info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		recreateSwapchain();
}


void VulkanSwapChain::recreateSwapchain()
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


void VulkanSwapChain::initialize()
{
	SwapChainSupportDetails swapDetails = querySwapchainSupport(*static_cast<VulkanRenderDevice*>(getDevice())->getPhysicalDevice(), mSurface);
	vk::SurfaceFormatKHR swapFormat = chooseSurfaceFormat(swapDetails.formats);
	vk::PresentModeKHR presMode = choosePresentMode(swapDetails.presentModes);
	ImageExtent swapExtent = chooseSwapExtent(mWindow);

	uint32_t images = 0;
	if (swapDetails.capabilities.maxImageCount == 0) { // some intel GPUs return max image count of 0 so work around this here
		images = swapDetails.capabilities.minImageCount + 1;
	}
	else {
		images = swapDetails.capabilities.minImageCount + 1 > swapDetails.capabilities.maxImageCount ? swapDetails.capabilities.maxImageCount : swapDetails.capabilities.minImageCount + 1;
	}

	vk::SwapchainCreateInfoKHR info{}; // fill out the format and presentation mode info
	info.setSurface(mSurface);
	info.setImageExtent(vk::Extent2D{ swapExtent.width, swapExtent.height });
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

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

	mSwapChain = device->createSwapchain(info);
	mSwapChainExtent = swapExtent;
	mSwapChainFormat = getBellImageFormat(swapFormat.format);

	auto swapChainImages = device->getSwapchainImages(mSwapChain);
	for (vk::Image VkImage : swapChainImages)
	{
		VulkanImage* image = new VulkanImage(getDevice(),
			VkImage,
			mSwapChainFormat,
			ImageUsage::ColourAttachment,
			std::max(swapExtent.width, 1u),
			std::max(swapExtent.height, 1u),
			1, 1, 1, 1, "SwapChain");

		mSwapChainImages.push_back(image);
	}

	createSwapChainImageViews();
}
