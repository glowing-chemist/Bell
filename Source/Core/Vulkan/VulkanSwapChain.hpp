#ifndef VK_SWAPCHAIN_HPP
#define VK_SWAPCHAIN_HPP

#include "Core/SwapChain.hpp"

#include <vulkan/vulkan.hpp>

struct SwapChainSupportDetails { // represent swapchain capabilities
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

class VulkanSwapChain : public SwapChainBase
{
public:
	VulkanSwapChain(RenderDevice* Device, vk::SurfaceKHR windowSurface, GLFWwindow* window);
	~VulkanSwapChain();

	virtual void destroy() override;

	virtual uint32_t getNextImageIndex() override;
	virtual void present(const QueueType queueIndex) override;

	const vk::Semaphore* getImageAquired() const
	{
		return &mImageAquired[getCurrentImageIndex()];
	}

	const vk::Semaphore* getImageRendered() const
	{
		return &mImageRendered[getCurrentImageIndex()];
	}

	vk::Semaphore* getImageAquired()
	{
		return &mImageAquired[getCurrentImageIndex()];
	}

	vk::Semaphore* getImageRendered()
	{
		return &mImageRendered[getCurrentImageIndex()];
	}

private:

	void initialize();
	void recreateSwapchain();
	void createSwapChainImageViews();

	SwapChainSupportDetails querySwapchainSupport(vk::PhysicalDevice dev, vk::SurfaceKHR surface);

	vk::SurfaceKHR mSurface;
	vk::SwapchainKHR mSwapChain;

	std::vector<vk::Semaphore> mImageAquired;
	std::vector<vk::Semaphore> mImageRendered;
};

#endif