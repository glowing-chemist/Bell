#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

struct VKRenderPasses;

// swapChain setup functions
struct SwapChainSupportDetails { // represent swapchain capabilities
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};


class SwapChain {
public:
    SwapChain(vk::Device& Device, vk::PhysicalDevice physDevice, vk::SurfaceKHR windowSurface, GLFWwindow* window);

    void destroy(vk::Device&);

    vk::Format getSwapChainImageFormat() const;

    unsigned int getSwapChainImageWidth() const;
    unsigned int getSwapChainImageHeight() const;

    unsigned int getNumberOfSwapChainImages() const;

    const vk::ImageView& getImageView(const size_t) const;
    vk::Image& getImage(const size_t);

	uint32_t getNextImageIndex(vk::Device&, vk::Semaphore&);
	uint32_t getCurrentImageIndex() { return mCurrentImageIndex; };

	void present(vk::Queue&, vk::Semaphore&);

private:
    SwapChainSupportDetails querySwapchainSupport(vk::PhysicalDevice, vk::SurfaceKHR);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR&, GLFWwindow*);

    void createSwapChainImageViews(vk::Device&);

	uint32_t mCurrentImageIndex;
    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    vk::Extent2D swapChainExtent;
    vk::Format swapChainFormat;
};

#endif
