#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include "DeviceChild.hpp"
#include "Core/Image.hpp"

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

struct VKRenderPasses;

// swapChain setup functions
struct SwapChainSupportDetails { // represent swapchain capabilities
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};


class SwapChain : public DeviceChild
{
public:
    SwapChain(RenderDevice* Device, vk::SurfaceKHR windowSurface, GLFWwindow* window);
    ~SwapChain();

    void destroy();

    vk::Format getSwapChainImageFormat() const;

    unsigned int getSwapChainImageWidth() const;
    unsigned int getSwapChainImageHeight() const;

    unsigned int getNumberOfSwapChainImages() const;

    ImageView& getImageView(const size_t index)
        { return mImageViews[index]; }

    Image& getImage(const size_t index)
        { return mSwapChainImages[index]; }

    const Image& getImage(const size_t index) const
        { return mSwapChainImages[index]; }

    uint32_t getNextImageIndex(vk::Semaphore&);
	uint32_t getCurrentImageIndex() const { return mCurrentImageIndex; }

	void present(vk::Queue&, vk::Semaphore&);

private:
    SwapChainSupportDetails querySwapchainSupport(vk::PhysicalDevice, vk::SurfaceKHR);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR&, GLFWwindow*);

    void createSwapChainImageViews();

	uint32_t mCurrentImageIndex;
    vk::SwapchainKHR mSwapChain;
    std::vector<Image> mSwapChainImages;
    std::vector<ImageView> mImageViews;
    vk::Extent2D mSwapChainExtent;
    vk::Format mSwapChainFormat;
};

#endif
