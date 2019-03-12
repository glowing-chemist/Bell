#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include "DeviceChild.hpp"

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

    void destroy();

    vk::Format getSwapChainImageFormat() const;

    unsigned int getSwapChainImageWidth() const;
    unsigned int getSwapChainImageHeight() const;

    unsigned int getNumberOfSwapChainImages() const;

    const vk::ImageView& getImageView(const size_t) const;
    vk::Image& getImage(const size_t);

    uint32_t getNextImageIndex(vk::Semaphore&);
	uint32_t getCurrentImageIndex() const { return mCurrentImageIndex; };

	void present(vk::Queue&, vk::Semaphore&);

private:
    SwapChainSupportDetails querySwapchainSupport(vk::PhysicalDevice, vk::SurfaceKHR);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR&, GLFWwindow*);

    void createSwapChainImageViews();

	uint32_t mCurrentImageIndex;
    vk::SwapchainKHR mSwapChain;
    std::vector<vk::Image> mSwapChainImages;
    std::vector<vk::ImageView> mSwapChainImageViews;
    vk::Extent2D mSwapChainExtent;
    vk::Format mSwapChainFormat;
};

#endif
