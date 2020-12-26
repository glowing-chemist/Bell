#include "VulkanBarrierManager.hpp"
#include "VulkanImage.hpp"
#include "VulkanImageView.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanBufferView.hpp"
#include "VulkanRenderDevice.hpp"
#include "Core/ConversionUtils.hpp"

#include <algorithm>


VulkanBarrierRecorder::VulkanBarrierRecorder(RenderDevice* device) :
	BarrierRecorderBase(device),
	mImageMemoryBarriers{},
	mBufferMemoryBarriers{},
	mMemoryBarriers{}
{}


void VulkanBarrierRecorder::transferResourceToQueue(Image& image, const QueueType queueType, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	if (queueType == image->getOwningQueueType())
		return;

	updateSyncPoints(src, dst);

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;

	switch (hazard)
	{
	case Hazard::ReadAfterWrite:
	{
		srcAccess = vk::AccessFlagBits::eShaderWrite;
		dstAccess = vk::AccessFlagBits::eShaderRead;
		break;
	}

	case Hazard::WriteAfterRead:
	{
		srcAccess = vk::AccessFlagBits::eShaderRead;
		dstAccess = vk::AccessFlagBits::eShaderWrite;
		break;
	}
	}

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());
	VulkanImage* VkImage = static_cast<VulkanImage*>(image.getBase());

	vk::ImageMemoryBarrier releaseBarrier{};
	releaseBarrier.setSrcQueueFamilyIndex(device->getQueueFamilyIndex(image->getOwningQueueType()));
	releaseBarrier.setDstQueueFamilyIndex(device->getQueueFamilyIndex(queueType));
	releaseBarrier.setSrcAccessMask(srcAccess);
	releaseBarrier.setDstAccessMask(dstAccess);
	releaseBarrier.setImage(VkImage->getImage());

	vk::ImageMemoryBarrier aquireBarrier{};
	aquireBarrier.setSrcQueueFamilyIndex(device->getQueueFamilyIndex(image->getOwningQueueType()));
	aquireBarrier.setDstQueueFamilyIndex(device->getQueueFamilyIndex(queueType));
	aquireBarrier.setSrcAccessMask(dstAccess);
	releaseBarrier.setDstAccessMask(srcAccess);
	aquireBarrier.setImage(VkImage->getImage());

	mImageMemoryBarriers.push_back({image->getOwningQueueType(), releaseBarrier});
	mImageMemoryBarriers.push_back({ queueType, aquireBarrier });

	image->setOwningQueueType(queueType);
}


void VulkanBarrierRecorder::transferResourceToQueue(Buffer& buffer, const QueueType queueType, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	if (queueType == buffer->getOwningQueueType())
		return;

	updateSyncPoints(src, dst);

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;

	switch (hazard)
	{
	case Hazard::ReadAfterWrite:
	{
		srcAccess = vk::AccessFlagBits::eMemoryWrite;
		dstAccess = vk::AccessFlagBits::eMemoryRead;

		if (buffer->getUsage() & BufferUsage::IndirectArgs)
			dstAccess |= vk::AccessFlagBits::eIndirectCommandRead;
		break;
	}

	case Hazard::WriteAfterRead:
	{
		srcAccess = vk::AccessFlagBits::eMemoryRead;
		dstAccess = vk::AccessFlagBits::eMemoryWrite;
		break;
	}
	}

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

	vk::BufferMemoryBarrier releaseBarrier{};
	releaseBarrier.setSrcQueueFamilyIndex(device->getQueueFamilyIndex(buffer->getOwningQueueType()));
	releaseBarrier.setDstQueueFamilyIndex(device->getQueueFamilyIndex(queueType));
	releaseBarrier.setSrcAccessMask(srcAccess);
	releaseBarrier.setDstAccessMask(dstAccess);
	releaseBarrier.setBuffer(static_cast<VulkanBuffer*>(buffer.getBase())->getBuffer());
	releaseBarrier.setSize(buffer->getSize());

	vk::BufferMemoryBarrier aquireBarrier{};
	aquireBarrier.setSrcQueueFamilyIndex(device->getQueueFamilyIndex(buffer->getOwningQueueType()));
	aquireBarrier.setDstQueueFamilyIndex(device->getQueueFamilyIndex(queueType));
	aquireBarrier.setSrcAccessMask(dstAccess);
	releaseBarrier.setDstAccessMask(srcAccess);
	aquireBarrier.setBuffer(static_cast<VulkanBuffer*>(buffer.getBase())->getBuffer());
	aquireBarrier.setSize(buffer->getSize());

	mBufferMemoryBarriers.push_back({ buffer->getOwningQueueType(), releaseBarrier });
	mBufferMemoryBarriers.push_back({ queueType, aquireBarrier });

	buffer->setOwningQueueType(queueType);
}


void VulkanBarrierRecorder::memoryBarrier(Image& img, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	ImageLayout layout = img->getLayout(0, 0);
	bool splitBarriers = true;
	for (uint32_t i = 0; i < img->numberOfLevels(); ++i)
	{
		for (uint32_t j = 0; j < img->numberOfMips(); ++j)
		{
			splitBarriers = splitBarriers && img->getLayout(i, j) != layout;
		}
	}

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;

	switch (hazard)
	{
	case Hazard::ReadAfterWrite:
	{
		srcAccess = vk::AccessFlagBits::eShaderWrite;
		dstAccess = vk::AccessFlagBits::eShaderRead;
		break;
	}

	case Hazard::WriteAfterRead:
	{
		srcAccess = vk::AccessFlagBits::eShaderRead;
		dstAccess = vk::AccessFlagBits::eShaderWrite;
		break;
	}
	}

	VulkanImage* VkImage = static_cast<VulkanImage*>(img.getBase());

	if (splitBarriers)
	{
		for (uint32_t i = 0; i < img->numberOfLevels(); ++i)
		{
			for (uint32_t j = 0; j < img->numberOfMips(); ++j)
			{
				vk::ImageMemoryBarrier barrier{};
				barrier.setSrcAccessMask(srcAccess);
				barrier.setDstAccessMask(dstAccess);
				barrier.setOldLayout(getVulkanImageLayout(img->getLayout(i, j)));
				barrier.setNewLayout(getVulkanImageLayout(img->getLayout(i, j)));
				if (img->getLayout(0, 0) == ImageLayout::DepthStencil ||
					img->getLayout(0, 0) == ImageLayout::DepthStencilRO)
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, j, 1, i, 1 });
				}
				else
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, j, 1, i, 1 });
				}
				barrier.setImage(VkImage->getImage());

				mImageMemoryBarriers.push_back({ img->getOwningQueueType(), barrier });
			}
		}
	}
	else
	{
		vk::ImageMemoryBarrier barrier{};
		barrier.setSrcAccessMask(srcAccess);
		barrier.setDstAccessMask(dstAccess);
		barrier.setOldLayout(getVulkanImageLayout(layout));
		barrier.setNewLayout(getVulkanImageLayout(layout));
		if (img->getLayout(0, 0) == ImageLayout::DepthStencil ||
			img->getLayout(0, 0) == ImageLayout::DepthStencilRO)
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, 0, img->numberOfMips(), 0, img->numberOfLevels() });
		}
		else
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, img->numberOfMips(), 0, img->numberOfLevels() });
		}
		barrier.setImage(VkImage->getImage());

		mImageMemoryBarriers.push_back({ img->getOwningQueueType(), barrier });
	}
}


void VulkanBarrierRecorder::memoryBarrier(ImageView& img, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	ImageLayout layout = img->getImageLayout(0, 0);
	bool splitBarriers = true;
	for (uint32_t i = img->getBaseLevel(); i < img->getBaseLevel() + img->getLevelCount(); ++i)
	{
		for (uint32_t j = img->getBaseMip(); j < img->getBaseMip() + img->getMipsCount(); ++j)
		{
			splitBarriers = splitBarriers && img->getImageLayout(i, j) != layout;
		}
	}

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;

	switch (hazard)
	{
	case Hazard::ReadAfterWrite:
	{
		srcAccess = vk::AccessFlagBits::eShaderWrite;
		dstAccess = vk::AccessFlagBits::eShaderRead;
		break;
	}

	case Hazard::WriteAfterRead:
	{
		srcAccess = vk::AccessFlagBits::eShaderRead;
		dstAccess = vk::AccessFlagBits::eShaderWrite;
		break;
	}
	}

	VulkanImageView* VkImage = static_cast<VulkanImageView*>(img.getBase());

	if (splitBarriers)
	{
		for (uint32_t i = img->getBaseLevel(); i < img->getBaseLevel() + img->getLevelCount(); ++i)
		{
			for (uint32_t j = img->getBaseMip(); j < img->getBaseMip() + img->getMipsCount(); ++j)
			{
				vk::ImageMemoryBarrier barrier{};
				barrier.setSrcAccessMask(srcAccess);
				barrier.setDstAccessMask(dstAccess);
				barrier.setOldLayout(getVulkanImageLayout(img->getImageLayout(i, j)));
				barrier.setNewLayout(getVulkanImageLayout(img->getImageLayout(i, j)));
				if (img->getImageLayout(0, 0) == ImageLayout::DepthStencil ||
					img->getImageLayout(0, 0) == ImageLayout::DepthStencilRO)
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, j, 1, i, 1 });
				}
				else
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, j, 1, i, 1 });
				}
				barrier.setImage(VkImage->getImage());

				mImageMemoryBarriers.push_back({ img->getOwningQueueType(), barrier });
			}
		}
	}
	else
	{
		vk::ImageMemoryBarrier barrier{};
		barrier.setSrcAccessMask(srcAccess);
		barrier.setDstAccessMask(dstAccess);
		barrier.setOldLayout(getVulkanImageLayout(layout));
		barrier.setNewLayout(getVulkanImageLayout(layout));
		if (img->getImageLayout(0, 0) == ImageLayout::DepthStencil ||
			img->getImageLayout(0, 0) == ImageLayout::DepthStencilRO)
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, img->getBaseMip(), img->getMipsCount(), img->getBaseLevel(), img->getLevelCount() });
		}
		else
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, img->getBaseMip(), img->getMipsCount(), img->getBaseLevel(), img->getLevelCount() });
		}
		barrier.setImage(VkImage->getImage());

		mImageMemoryBarriers.push_back({ img->getOwningQueueType(), barrier });
	}
}


void VulkanBarrierRecorder::memoryBarrier(Buffer& buf, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;

	switch (hazard)
	{
	case Hazard::ReadAfterWrite:
	{
		srcAccess = vk::AccessFlagBits::eMemoryWrite;
		dstAccess = vk::AccessFlagBits::eMemoryRead;

		if (buf->getUsage() & BufferUsage::IndirectArgs)
			dstAccess |= vk::AccessFlagBits::eIndirectCommandRead;
		break;
	}

	case Hazard::WriteAfterRead:
	{
		srcAccess = vk::AccessFlagBits::eMemoryRead;
		dstAccess = vk::AccessFlagBits::eMemoryWrite;
		break;
	}
	}

	vk::BufferMemoryBarrier barrier{};
	barrier.setSrcAccessMask(srcAccess);
	barrier.setDstAccessMask(dstAccess);
	barrier.setBuffer(static_cast<VulkanBuffer*>(buf.getBase())->getBuffer());
	barrier.setSize(buf->getSize());

	mBufferMemoryBarriers.push_back({ buf->getOwningQueueType(), barrier });
}


void VulkanBarrierRecorder::memoryBarrier(BufferView& buf, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;

	switch (hazard)
	{
	case Hazard::ReadAfterWrite:
	{
		srcAccess = vk::AccessFlagBits::eMemoryWrite;
		dstAccess = vk::AccessFlagBits::eMemoryRead;

        if (buf->getUsage() & BufferUsage::IndirectArgs && dst == SyncPoint::IndirectArgs)
			dstAccess |= vk::AccessFlagBits::eIndirectCommandRead;
		break;
	}

	case Hazard::WriteAfterRead:
	{
		srcAccess = vk::AccessFlagBits::eMemoryRead;
		dstAccess = vk::AccessFlagBits::eMemoryWrite;
		break;
	}
	}

	vk::BufferMemoryBarrier barrier{};
	barrier.setSrcAccessMask(srcAccess);
	barrier.setDstAccessMask(dstAccess);
	barrier.setBuffer(static_cast<VulkanBufferView*>(buf.getBase())->getBuffer());
	barrier.setSize(buf->getSize());
	barrier.setOffset(buf->getOffset());

	mBufferMemoryBarriers.push_back({ QueueType::Graphics, barrier });
}


void VulkanBarrierRecorder::memoryBarrier(const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;
	switch (hazard)
	{
	case Hazard::ReadAfterWrite:
	{
		srcAccess = vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eShaderWrite;
		dstAccess = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eIndirectCommandRead;

		break;
	}

	case Hazard::WriteAfterRead:
	{
		srcAccess = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eShaderRead;
		dstAccess = vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eShaderWrite;
		break;
	}
	}

	vk::MemoryBarrier barrier{srcAccess, dstAccess};

	mMemoryBarriers.push_back({ QueueType::Graphics ,barrier });

}


void VulkanBarrierRecorder::transitionLayout(Image& img, const ImageLayout newLayout, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	std::vector<ImageLayout> oldLayouts{};
	oldLayouts.resize(img->mNumberOfLevels * img->mNumberOfMips);
	ImageLayout layout = img->getLayout(0, 0);
	bool splitBarriers = true;
	for (uint32_t i = 0; i < img->numberOfLevels(); ++i)
	{
		for (uint32_t j = 0; j < img->numberOfMips(); ++j)
		{
			ImageLayout oldLayout = img->getLayout(i, j);
			splitBarriers = splitBarriers || oldLayout != layout;
			oldLayouts[(i * img->numberOfMips()) + j] = oldLayout;
			(*img->mSubResourceInfo)[(i * img->numberOfMips()) + j].mLayout = newLayout;
		}
	}

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;

	switch (hazard)
	{
	case Hazard::ReadAfterWrite:
	{
		if (src == SyncPoint::VertexShader ||
			src == SyncPoint::FragmentShader ||
			src == SyncPoint::ComputeShader)
			srcAccess = vk::AccessFlagBits::eShaderWrite;
		else
			srcAccess = vk::AccessFlagBits::eMemoryWrite;

		if (dst == SyncPoint::VertexShader ||
			dst == SyncPoint::FragmentShader ||
			dst == SyncPoint::ComputeShader)
			dstAccess = vk::AccessFlagBits::eShaderRead;
		else
			dstAccess = vk::AccessFlagBits::eMemoryRead;

		break;
	}

	case Hazard::WriteAfterRead:
	{
		if (src == SyncPoint::VertexShader ||
			src == SyncPoint::FragmentShader ||
			src == SyncPoint::ComputeShader)
			srcAccess = vk::AccessFlagBits::eShaderRead;
		else
			srcAccess = vk::AccessFlagBits::eMemoryRead;

		if (dst == SyncPoint::VertexShader ||
			dst == SyncPoint::FragmentShader ||
			dst == SyncPoint::ComputeShader)
			dstAccess = vk::AccessFlagBits::eShaderWrite;
		else
			srcAccess = vk::AccessFlagBits::eMemoryWrite;

		break;
	}
	}

	if (splitBarriers)
	{
		for (uint32_t i = 0; i < img->numberOfLevels(); ++i)
		{
			for (uint32_t j = 0; j < img->numberOfMips(); ++j)
			{
				vk::ImageMemoryBarrier barrier{};
				barrier.setSrcAccessMask(srcAccess);
				barrier.setDstAccessMask(dstAccess);
				barrier.setOldLayout(getVulkanImageLayout(oldLayouts[(i * img->numberOfMips()) + j]));
				barrier.setNewLayout(getVulkanImageLayout(newLayout));
				if (newLayout == ImageLayout::DepthStencil ||
					newLayout == ImageLayout::DepthStencilRO)
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, j, 1, i, 1 });
				}
				else
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, j, 1, i, 1 });
				}
				barrier.setImage(static_cast<VulkanImage*>(img.getBase())->getImage());

				(*img->mSubResourceInfo)[(i * img->numberOfMips()) + j].mLayout = newLayout;

				mImageMemoryBarriers.push_back({ img->getOwningQueueType(), barrier });
			}
		}
	}
	else
	{
		vk::ImageMemoryBarrier barrier{};
		barrier.setSrcAccessMask(srcAccess);
		barrier.setDstAccessMask(dstAccess);
		barrier.setOldLayout(getVulkanImageLayout(layout));
		barrier.setNewLayout(getVulkanImageLayout(newLayout));
		if (newLayout == ImageLayout::DepthStencil ||
			newLayout == ImageLayout::DepthStencilRO)
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, 0, img->numberOfMips(), 0, img->numberOfLevels() });
		}
		else
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, img->numberOfMips(), 0, img->numberOfLevels() });
		}
		barrier.setImage(static_cast<VulkanImage*>(img.getBase())->getImage());

		mImageMemoryBarriers.push_back({ img->getOwningQueueType(), barrier });
	}
}


void VulkanBarrierRecorder::transitionLayout(ImageView& img, const ImageLayout newLayout, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	std::vector<ImageLayout> oldLayouts{};
	oldLayouts.resize(img->getTotalSubresourceCount());
    ImageLayout layout = img->getImageLayout(img->getBaseLevel(), img->getBaseMip());
	bool splitBarriers = false;
	for (uint32_t i = img->getBaseLevel(); i < img->getBaseLevel() + img->getLevelCount(); ++i)
	{
		for (uint32_t j = img->getBaseMip(); j < img->getBaseMip() + img->getMipsCount(); ++j)
		{
			ImageLayout oldLayout = img->getImageLayout(i, j);
			splitBarriers = splitBarriers || oldLayout != layout;
			oldLayouts[(i * img->mTotalMips) + j] = oldLayout;
			img->mSubResourceInfo[(i * img->mTotalMips) + j].mLayout = newLayout;
		}
	}

	vk::AccessFlags srcAccess;
	vk::AccessFlags dstAccess;

	switch (hazard)
	{
	case Hazard::ReadAfterWrite:
	{
		if (src == SyncPoint::VertexShader ||
			src == SyncPoint::FragmentShader ||
			src == SyncPoint::ComputeShader)
			srcAccess = vk::AccessFlagBits::eShaderWrite;
		else
			srcAccess = vk::AccessFlagBits::eMemoryWrite;

		if (dst == SyncPoint::VertexShader ||
			dst == SyncPoint::FragmentShader ||
			dst == SyncPoint::ComputeShader)
			dstAccess = vk::AccessFlagBits::eShaderRead;
		else
			srcAccess = vk::AccessFlagBits::eMemoryRead;

		break;
	}

	case Hazard::WriteAfterRead:
	{
		if (src == SyncPoint::VertexShader ||
			src == SyncPoint::FragmentShader ||
			src == SyncPoint::ComputeShader)
			srcAccess = vk::AccessFlagBits::eShaderRead;
		else
			srcAccess = vk::AccessFlagBits::eMemoryRead;

		if (dst == SyncPoint::VertexShader ||
			dst == SyncPoint::FragmentShader ||
			dst == SyncPoint::ComputeShader)
			dstAccess = vk::AccessFlagBits::eShaderWrite;
		else
			srcAccess = vk::AccessFlagBits::eMemoryWrite;

		break;
	}
	}

	if (splitBarriers)
	{
		for (uint32_t i = img->getBaseLevel(); i < img->getBaseLevel() + img->getLevelCount(); ++i)
		{
			for (uint32_t j = img->getBaseMip(); j < img->getBaseMip() + img->getMipsCount(); ++j)
			{
				vk::ImageMemoryBarrier barrier{};
				barrier.setSrcAccessMask(srcAccess);
				barrier.setDstAccessMask(dstAccess);
				barrier.setOldLayout(getVulkanImageLayout(oldLayouts[(i * img->mTotalMips) + j]));
				barrier.setNewLayout(getVulkanImageLayout(newLayout));
				if (newLayout == ImageLayout::DepthStencil ||
					newLayout == ImageLayout::DepthStencilRO)
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, j, 1, i, 1 });
				}
				else
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, j, 1, i, 1 });
				}
				barrier.setImage(static_cast<VulkanImageView*>(img.getBase())->getImage());

				mImageMemoryBarriers.push_back({ img->getOwningQueueType(), barrier });
			}
		}
	}
	else
	{
		vk::ImageMemoryBarrier barrier{};
		barrier.setSrcAccessMask(srcAccess);
		barrier.setDstAccessMask(dstAccess);
		barrier.setOldLayout(getVulkanImageLayout(layout));
		barrier.setNewLayout(getVulkanImageLayout(newLayout));
		if (newLayout == ImageLayout::DepthStencil ||
			newLayout == ImageLayout::DepthStencilRO)
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, img->getBaseMip(), img->getMipsCount(), img->getBaseLevel(), img->getLevelCount() });
		}
		else
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, img->getBaseMip(), img->getMipsCount(), img->getBaseLevel(), img->getLevelCount() });
		}
		barrier.setImage(static_cast<VulkanImageView*>(img.getBase())->getImage());

		mImageMemoryBarriers.push_back({ img->getOwningQueueType(), barrier });
	}
}


void VulkanBarrierRecorder::signalAsyncQueueSemaphore(const uint64_t val)
{
    mSemaphoreOps.push_back({true, val});
}

void VulkanBarrierRecorder::waitOnAsyncQueueSemaphore(const uint64_t val)
{
    mSemaphoreOps.push_back({false, val});
}


std::vector<vk::ImageMemoryBarrier> VulkanBarrierRecorder::getImageBarriers(const QueueType type) const
{
	std::vector<vk::ImageMemoryBarrier> barriers;

	for (auto [queueType, barrier] : mImageMemoryBarriers)
	{
		if (queueType == type)
			barriers.push_back(barrier);
	}

	return barriers;
}


std::vector<vk::BufferMemoryBarrier> VulkanBarrierRecorder::getBufferBarriers(const QueueType type) const
{
	std::vector<vk::BufferMemoryBarrier> barriers;

	for (auto [queueType, barrier] : mBufferMemoryBarriers)
	{
		if (queueType == type)
			barriers.push_back(barrier);
	}

	return barriers;
}


std::vector<vk::MemoryBarrier> VulkanBarrierRecorder::getMemoryBarriers(const QueueType type) const
{
	std::vector<vk::MemoryBarrier> barriers;

	for (auto [queueType, barrier] : mMemoryBarriers)
	{
		if (queueType == type)
			barriers.push_back(barrier);
	}

	return barriers;
}
