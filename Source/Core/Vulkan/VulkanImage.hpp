#ifndef VULKAN_IMAGE_HPP
#define VULKAN_IMAGE_HPP

#include <vulkan/vulkan.hpp>

#include "Core/Image.hpp"
#include "MemoryManager.hpp"


class VulkanImage : public ImageBase
{
public:

	VulkanImage(RenderDevice* dev,
		const Format format,
		const ImageUsage usage,
		const uint32_t x,
		const uint32_t y,
		const uint32_t z = 1,
		const uint32_t mips = 1,
		const uint32_t levels = 1,
		const uint32_t samples = 1,
		const std::string& debugName = "");

	VulkanImage(RenderDevice* dev,
		vk::Image& VulkanImage,
		Format format,
		const ImageUsage usage,
		const uint32_t x,
		const uint32_t y,
		const uint32_t z = 1,
		const uint32_t mips = 1,
		const uint32_t levels = 1,
		const uint32_t samples = 1,
		const std::string& debugName = "");

	~VulkanImage();

	virtual void swap(ImageBase&) override; // used instead of a move constructor.

	virtual void setContents(const void* data,
		const uint32_t xsize,
		const uint32_t ysize,
		const uint32_t zsize,
		const uint32_t level = 0,
		const uint32_t lod = 0,
		const int32_t offsetx = 0,
		const int32_t offsety = 0,
		const int32_t offsetz = 0) override;

	virtual void clear() override;


	vk::Image getImage() const
	{
		return mImage;
	}

	Allocation getMemory() const
	{
		return mImageMemory;
	}

private:
	vk::Image mImage;
	Allocation mImageMemory;
	vk::ImageType mType;

	bool mIsOwned;
};

#endif