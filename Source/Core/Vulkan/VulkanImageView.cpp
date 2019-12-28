#include "VulkanImageView.hpp"
#include "VulkanImage.hpp"
#include "VulkanRenderDevice.hpp"
#include "Core/ConversionUtils.hpp"


VulkanImageView::VulkanImageView(Image& parentImage,
					 const ImageViewType viewType,
					 const uint32_t level,
					 const uint32_t levelCount,
					 const uint32_t mip,
					 const uint32_t mipCount) :
    ImageViewBase(parentImage, viewType, level, levelCount, mip, mipCount),
    mImage{static_cast<VulkanImage&>(*parentImage.getBase()).getImage()},
    mImageMemory{ static_cast<VulkanImage&>(*parentImage.getBase()).getMemory()}
{
	const vk::ImageAspectFlags adjustedViewType = [viewType]()
	{
		switch(viewType)
		{
			case ImageViewType::Colour:
				return vk::ImageAspectFlagBits::eColor;

			case ImageViewType::Depth:
				return vk::ImageAspectFlagBits::eDepth;
		}
	}();

    vk::ImageSubresourceRange subresourceRange{};
	subresourceRange.setBaseMipLevel(mip);
	subresourceRange.setLevelCount(mipCount);
    subresourceRange.setBaseArrayLayer(level);
	subresourceRange.setLayerCount(mLayerEnd);
	subresourceRange.setAspectMask(adjustedViewType);

	const auto extent = parentImage->getExtent(mLayerStart, mMipStart);

	vk::ImageViewType type = vk::ImageViewType::e1D;

	if(extent.height > 0)
        type = vk::ImageViewType::e2D;

	if(extent.height > 0 && levelCount > 1)
		type = vk::ImageViewType::e2DArray;

    if(extent.depth > 1)
        type = vk::ImageViewType::e3D;

	if(parentImage->getUsage() & ImageUsage::CubeMap)
		type = vk::ImageViewType::eCube;


    vk::ImageViewCreateInfo createInfo{};
    createInfo.setImage(mImage);
	createInfo.setFormat(getVulkanImageFormat(mImageFormat));
    createInfo.setSubresourceRange(subresourceRange);
    createInfo.setViewType(type);

    mView = static_cast<VulkanRenderDevice*>(getDevice())->createImageView(createInfo);
}


VulkanImageView::~VulkanImageView()
{
	mImage = nullptr;
	getDevice()->destroyImageView(*this);
}
