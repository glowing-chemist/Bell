#include "SwapChain.hpp"
#include "RenderInstance.hpp"
#include "RenderDevice.hpp"

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <limits>

vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
    if(formats.size() == 1 && formats[0].format == vk::Format::eUndefined) { // indicates any format can be used, so pick the best
        vk::SurfaceFormatKHR form;
        form.format = vk::Format::eB8G8R8A8Unorm;
        form.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        return form;
    }
    return formats[0]; // if not pick the first format
}

unsigned int SwapChain::getSwapChainImageWidth() const {
    return mSwapChainExtent.width;
}


unsigned int SwapChain::getSwapChainImageHeight() const {
    return mSwapChainExtent.height;
}


unsigned int SwapChain::getNumberOfSwapChainImages() const {
    return mSwapChainImages.size();
}


SwapChainSupportDetails SwapChain::querySwapchainSupport(vk::PhysicalDevice dev, vk::SurfaceKHR surface) {
    SwapChainSupportDetails details;

    details.capabilities = dev.getSurfaceCapabilitiesKHR(surface);
    details.formats = dev.getSurfaceFormatsKHR(surface);
    details.presentModes = dev.getSurfacePresentModesKHR(surface);

    return details;
}

vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes) {
    for(auto& mode : presentModes) {
        if(mode == vk::PresentModeKHR::eMailbox) { // if availble return mailbox
            return mode;
        }
    }
    return vk::PresentModeKHR::eFifo; // else Efifo is garenteed to be present
}

vk::Extent2D SwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR&, GLFWwindow* window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    return vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

SwapChain::SwapChain(RenderDevice* Device, vk::SurfaceKHR windowSurface, GLFWwindow* window) :
    DeviceChild{Device},
    mCurrentImageIndex{0}
{
    SwapChainSupportDetails swapDetails = querySwapchainSupport(*getDevice()->getPhysicalDevice(), windowSurface);
    vk::SurfaceFormatKHR swapFormat = chooseSurfaceFormat(swapDetails.formats);
    vk::PresentModeKHR presMode = choosePresentMode(swapDetails.presentModes);
    vk::Extent2D swapExtent = chooseSwapExtent(swapDetails.capabilities, window);

    uint32_t images = 0;
    if(swapDetails.capabilities.maxImageCount == 0) { // some intel GPUs return max image count of 0 so work around this here
        images = swapDetails.capabilities.minImageCount + 1;
    } else {
        images = swapDetails.capabilities.minImageCount + 1 > swapDetails.capabilities.maxImageCount ? swapDetails.capabilities.maxImageCount: swapDetails.capabilities.minImageCount + 1;
    }

    vk::SwapchainCreateInfoKHR info{}; // fill out the format and presentation mode info
    info.setSurface(windowSurface);
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

    mSwapChainImages = getDevice()->getSwapchainImages(mSwapChain);

    mSwapChainExtent = swapExtent;
    mSwapChainFormat = swapFormat.format;

    createSwapChainImageViews();
}


void SwapChain::destroy() {
    for(auto& imageView : mSwapChainImageViews) {
        getDevice()->destroyImageView(imageView);
    }
    getDevice()->destroySwapchain(mSwapChain);
}


vk::Format SwapChain::getSwapChainImageFormat() const {
    return mSwapChainFormat;
}


void SwapChain::createSwapChainImageViews() {
    mSwapChainImageViews.resize(mSwapChainImages.size());
    for(size_t i = 0; i < mSwapChainImages.size(); i++) {
        vk::ImageViewCreateInfo info{};
        info.setImage(mSwapChainImages[i]);
        info.setViewType(vk::ImageViewType::e2D);
        info.setFormat(mSwapChainFormat);

        info.setComponents(vk::ComponentMapping()); // set swizzle components to identity
        info.setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

        mSwapChainImageViews[i] = getDevice()->createImageView(info);
    }
    std::cerr << "created " << mSwapChainImageViews.size() << " image views" << std::endl;
}


const vk::ImageView& SwapChain::getImageView(const size_t index) const {
    return mSwapChainImageViews[index];
}


vk::Image& SwapChain::getImage(const size_t index) {
    return mSwapChainImages[index];
}


uint32_t SwapChain::getNextImageIndex(vk::Semaphore& sem) {
    getDevice()->aquireNextSwapchainImage(mSwapChain, std::numeric_limits<uint64_t>::max(), sem, mCurrentImageIndex);
	
	return mCurrentImageIndex;
}


void SwapChain::present(vk::Queue& presentQueue, vk::Semaphore& waitSemaphore) {
	vk::PresentInfoKHR info{};
    info.setPSwapchains(&mSwapChain);
	info.setSwapchainCount(1);
    info.setPWaitSemaphores(&waitSemaphore);
    info.setWaitSemaphoreCount(1);
	info.setPImageIndices(&mCurrentImageIndex);

	presentQueue.presentKHR(info);
}
