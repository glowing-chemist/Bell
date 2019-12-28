#ifndef VK_IMAGE_VIEW_HPP
#define VK_IMAGE_VIEW_HPP

#include <vulkan/vulkan.hpp>
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "MemoryManager.hpp"


class VulkanImageView : public ImageViewBase
{
public:

	VulkanImageView(Image&,
		const ImageViewType,
		const uint32_t level = 0,
		const uint32_t levelCount = 1,
		const uint32_t lod = 0,
		const uint32_t lodCount = 1);
	~VulkanImageView();

	vk::ImageView getImageView() const
	{
		return mView;
	}

	vk::Image getImage() const
	{
		return mImage;
	}

private:
	vk::Image mImage;
	vk::ImageView mView;
	Allocation mImageMemory;
};

#endif