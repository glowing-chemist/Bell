#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include "DeviceChild.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"

class RenderDevice;
struct GLFWwindow;


class SwapChainBase : public DeviceChild
{
public:
	SwapChainBase(RenderDevice* Device, GLFWwindow* window);
    virtual ~SwapChainBase() = default;

	virtual void destroy() {};

	virtual uint32_t getNextImageIndex() = 0;
	virtual void present(const QueueType queueIndex) = 0;

	Format getSwapChainImageFormat() const
	{
		return mSwapChainFormat;
	}

	uint32_t getSwapChainImageWidth() const;
	uint32_t getSwapChainImageHeight() const;

	virtual uint32_t getNumberOfSwapChainImages() const;

    virtual ImageView& getImageView(const size_t index)
        { return mImageViews[index]; }

	virtual const ImageView& getImageView(const size_t index) const
		{ return mImageViews[index]; }

	virtual Image& getImage(const size_t index)
        { return mSwapChainImages[index]; }

	virtual const Image& getImage(const size_t index) const
        { return mSwapChainImages[index]; }

	uint32_t getCurrentImageIndex() const { return mCurrentImageIndex; }

	ImageExtent chooseSwapExtent(GLFWwindow* window);

protected:

	uint32_t mCurrentImageIndex;
	GLFWwindow* mWindow;
    std::vector<Image> mSwapChainImages;
    std::vector<ImageView> mImageViews;
    ImageExtent mSwapChainExtent;
    Format mSwapChainFormat;
};

#endif
