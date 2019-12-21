#include "BarrierManager.hpp"
#include "Core/Image.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"
#include "Core/ConversionUtils.hpp"
#include "RenderDevice.hpp"

#include <algorithm>


BarrierRecorder::BarrierRecorder(RenderDevice* device) : 
	DeviceChild(device),
	mImageMemoryBarriers{},
	mBufferMemoryBarriers{},
	mMemoryBarriers{},
	mSrc{SyncPoint::TopOfPipe},
	mDst{ SyncPoint::BottomOfPipe }
{}


void BarrierRecorder::transferResourceToQueue(Image& image, const QueueType queueType, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	if (queueType == image.getOwningQueueType())
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

	vk::ImageMemoryBarrier releaseBarrier{};
	releaseBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(image.getOwningQueueType()));
	releaseBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	releaseBarrier.setSrcAccessMask(srcAccess);
	releaseBarrier.setDstAccessMask(dstAccess);
	releaseBarrier.setImage(image.getImage());

	vk::ImageMemoryBarrier aquireBarrier{};
	aquireBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(image.getOwningQueueType()));
	aquireBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	aquireBarrier.setSrcAccessMask(dstAccess);
	releaseBarrier.setDstAccessMask(srcAccess);
	aquireBarrier.setImage(image.getImage());

	mImageMemoryBarriers.push_back({image.getOwningQueueType(), releaseBarrier});
	mImageMemoryBarriers.push_back({ queueType, aquireBarrier });

	image.setOwningQueueType(queueType);
}


void BarrierRecorder::transferResourceToQueue(Buffer& buffer, const QueueType queueType, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	if (queueType == buffer.getOwningQueueType())
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

		if (buffer.getUsage() & BufferUsage::IndirectArgs)
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

	vk::BufferMemoryBarrier releaseBarrier{};
	releaseBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(buffer.getOwningQueueType()));
	releaseBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	releaseBarrier.setSrcAccessMask(srcAccess);
	releaseBarrier.setDstAccessMask(dstAccess);
	releaseBarrier.setBuffer(buffer.getBuffer());
	releaseBarrier.setSize(buffer.getSize());

	vk::BufferMemoryBarrier aquireBarrier{};
	aquireBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(buffer.getOwningQueueType()));
	aquireBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	aquireBarrier.setSrcAccessMask(dstAccess);
	releaseBarrier.setDstAccessMask(srcAccess);
	aquireBarrier.setBuffer(buffer.getBuffer());
	aquireBarrier.setSize(buffer.getSize());

	mBufferMemoryBarriers.push_back({ buffer.getOwningQueueType(), releaseBarrier });
	mBufferMemoryBarriers.push_back({ queueType, aquireBarrier });

	buffer.setOwningQueueType(queueType);
}


void BarrierRecorder::memoryBarrier(Image& img, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	ImageLayout layout = img.getLayout(0, 0);
	bool splitBarriers = true;
	for (uint32_t i = 0; i < img.numberOfLevels(); ++i)
	{
		for (uint32_t j = 0; j < img.numberOfMips(); ++j)
		{
			splitBarriers = splitBarriers && img.getLayout(i, j) != layout;
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

	if (splitBarriers)
	{
		for (uint32_t i = 0; i < img.numberOfLevels(); ++i)
		{
			for (uint32_t j = 0; j < img.numberOfMips(); ++j)
			{
				vk::ImageMemoryBarrier barrier{};
				barrier.setSrcAccessMask(srcAccess);
				barrier.setDstAccessMask(dstAccess);
				barrier.setOldLayout(getVulkanImageLayout(img.getLayout(i, j)));
				barrier.setNewLayout(getVulkanImageLayout(img.getLayout(i, j)));
				if (img.getLayout(0, 0) == ImageLayout::DepthStencil ||
					img.getLayout(0, 0) == ImageLayout::DepthStencilRO)
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, j, 1, i, 1 });
				}
				else
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, j, 1, i, 1 });
				}

				mImageMemoryBarriers.push_back({ img.getOwningQueueType(), barrier });
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
		if (img.getLayout(0, 0) == ImageLayout::DepthStencil ||
			img.getLayout(0, 0) == ImageLayout::DepthStencilRO)
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, 0, img.numberOfMips(), 0, img.numberOfLevels() });
		}
		else
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, img.numberOfMips(), 0, img.numberOfLevels() });
		}

		mImageMemoryBarriers.push_back({ img.getOwningQueueType(), barrier });
	}
}


void BarrierRecorder::memoryBarrier(ImageView& img, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	ImageLayout layout = img.getImageLayout(0, 0);
	bool splitBarriers = true;
	for (uint32_t i = img.getBaseLevel(); i < img.getBaseLevel() + img.getLevelCount(); ++i)
	{
		for (uint32_t j = img.getBaseMip(); j < img.getBaseMip() + img.getMipsCount(); ++j)
		{
			splitBarriers = splitBarriers && img.getImageLayout(i, j) != layout;
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

	if (splitBarriers)
	{
		for (uint32_t i = img.getBaseLevel(); i < img.getBaseLevel() + img.getLevelCount(); ++i)
		{
			for (uint32_t j = img.getBaseMip(); j < img.getBaseMip() + img.getMipsCount(); ++j)
			{
				vk::ImageMemoryBarrier barrier{};
				barrier.setSrcAccessMask(srcAccess);
				barrier.setDstAccessMask(dstAccess);
				barrier.setOldLayout(getVulkanImageLayout(img.getImageLayout(i, j)));
				barrier.setNewLayout(getVulkanImageLayout(img.getImageLayout(i, j)));
				if (img.getImageLayout(0, 0) == ImageLayout::DepthStencil ||
					img.getImageLayout(0, 0) == ImageLayout::DepthStencilRO)
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, j, 1, i, 1 });
				}
				else
				{
					barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, j, 1, i, 1 });
				}

				mImageMemoryBarriers.push_back({ img.getOwningQueueType(), barrier });
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
		if (img.getImageLayout(0, 0) == ImageLayout::DepthStencil ||
			img.getImageLayout(0, 0) == ImageLayout::DepthStencilRO)
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, img.getBaseMip(), img.getMipsCount(), img.getBaseLevel(), img.getLevelCount() });
		}
		else
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, img.getBaseMip(), img.getMipsCount(), img.getBaseLevel(), img.getLevelCount() });
		}

		mImageMemoryBarriers.push_back({ img.getOwningQueueType(), barrier });
	}
}


void BarrierRecorder::memoryBarrier(Buffer& buf, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
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

		if (buf.getUsage() & BufferUsage::IndirectArgs)
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
	barrier.setBuffer(buf.getBuffer());
	barrier.setSize(buf.getSize());

	mBufferMemoryBarriers.push_back({ buf.getOwningQueueType(), barrier });
}


void BarrierRecorder::memoryBarrier(BufferView& buf, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
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

		if (buf.getUsage() & BufferUsage::IndirectArgs)
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
	barrier.setBuffer(buf.getBuffer());
	barrier.setSize(buf.getSize());
	barrier.setOffset(buf.getOffset());

	mBufferMemoryBarriers.push_back({ QueueType::Graphics, barrier });
}


void BarrierRecorder::memoryBarrier(const Hazard hazard, const SyncPoint src, const SyncPoint dst)
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


void BarrierRecorder::transitionLayout(Image& img, const ImageLayout newLayout, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	ImageLayout layout = img.getLayout(0, 0);
	bool splitBarriers = true;
	for (uint32_t i = 0; i < img.numberOfLevels(); ++i)
	{
		for (uint32_t j = 0; j < img.numberOfMips(); ++j)
		{
			splitBarriers = splitBarriers && img.getLayout(i, j) != layout;
			(*img.mSubResourceInfo)[(i * img.numberOfMips()) + j].mLayout = newLayout;
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
		for (uint32_t i = 0; i < img.numberOfLevels(); ++i)
		{
			for (uint32_t j = 0; j < img.numberOfMips(); ++j)
			{
				vk::ImageMemoryBarrier barrier{};
				barrier.setSrcAccessMask(srcAccess);
				barrier.setDstAccessMask(dstAccess);
				barrier.setOldLayout(getVulkanImageLayout(img.getLayout(i, j)));
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
				barrier.setImage(img.getImage());

				mImageMemoryBarriers.push_back({ img.getOwningQueueType(), barrier });
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
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, 0, img.numberOfMips(), 0, img.numberOfLevels() });
		}
		else
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, img.numberOfMips(), 0, img.numberOfLevels() });
		}
		barrier.setImage(img.getImage());

		mImageMemoryBarriers.push_back({ img.getOwningQueueType(), barrier });
	}
}


void BarrierRecorder::transitionLayout(ImageView& img, const ImageLayout newLayout, const Hazard hazard, const SyncPoint src, const SyncPoint dst)
{
	updateSyncPoints(src, dst);

	ImageLayout layout = img.getImageLayout(0, 0);
	bool splitBarriers = true;
	for (uint32_t i = img.getBaseLevel(); i < img.getBaseLevel() + img.getLevelCount(); ++i)
	{
		for (uint32_t j = img.getBaseMip(); j < img.getBaseMip() + img.getMipsCount(); ++j)
		{
			splitBarriers = splitBarriers && img.getImageLayout(i, j) != layout;
			img.mSubResourceInfo[(i * img.mTotalMips) + j].mLayout = newLayout;
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
		for (uint32_t i = img.getBaseLevel(); i < img.getBaseLevel() + img.getLevelCount(); ++i)
		{
			for (uint32_t j = img.getBaseMip(); j < img.getBaseMip() + img.getMipsCount(); ++j)
			{
				vk::ImageMemoryBarrier barrier{};
				barrier.setSrcAccessMask(srcAccess);
				barrier.setDstAccessMask(dstAccess);
				barrier.setOldLayout(getVulkanImageLayout(img.getImageLayout(i, j)));
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
				barrier.setImage(img.getImage());

				mImageMemoryBarriers.push_back({ img.getOwningQueueType(), barrier });
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
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, img.getBaseMip(), img.getMipsCount(), img.getBaseLevel(), img.getLevelCount() });
		}
		else
		{
			barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, img.getBaseMip(), img.getMipsCount(), img.getBaseLevel(), img.getLevelCount() });
		}
		barrier.setImage(img.getImage());

		mImageMemoryBarriers.push_back({ img.getOwningQueueType(), barrier });
	}
}


std::vector<vk::ImageMemoryBarrier> BarrierRecorder::getImageBarriers(const QueueType type) const
{
	std::vector<vk::ImageMemoryBarrier> barriers;

	for (auto [queueType, barrier] : mImageMemoryBarriers)
	{
		if (queueType == type)
			barriers.push_back(barrier);
	}

	return barriers;
}


std::vector<vk::BufferMemoryBarrier> BarrierRecorder::getBufferBarriers(const QueueType type) const
{
	std::vector<vk::BufferMemoryBarrier> barriers;

	for (auto [queueType, barrier] : mBufferMemoryBarriers)
	{
		if (queueType == type)
			barriers.push_back(barrier);
	}

	return barriers;
}


std::vector<vk::MemoryBarrier> BarrierRecorder::getMemoryBarriers(const QueueType type) const
{
	std::vector<vk::MemoryBarrier> barriers;

	for (auto [queueType, barrier] : mMemoryBarriers)
	{
		if (queueType == type)
			barriers.push_back(barrier);
	}

	return barriers;
}


void BarrierRecorder::updateSyncPoints(const SyncPoint src, const SyncPoint dst)
{
	mSrc = static_cast<SyncPoint>(std::max(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(src)));
	mDst = static_cast<SyncPoint>(std::min(static_cast<uint32_t>(mDst), static_cast<uint32_t>(dst)));
}