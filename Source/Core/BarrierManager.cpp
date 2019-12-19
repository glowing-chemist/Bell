#include "BarrierManager.hpp"
#include "Core/Image.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"
#include "RenderDevice.hpp"

#include <algorithm>


BarrierRecorder::BarrierRecorder(RenderDevice* device) : 
	DeviceChild(device),
	mImageMemoryBarriers{},
	mBufferMemoryBarriers{},
	mMemoryBarriers{},
	mSrc{vk::PipelineStageFlagBits::eBottomOfPipe},
	mDst{ vk::PipelineStageFlagBits::eTopOfPipe }
{}


void BarrierRecorder::transferResourceToQueue(Image& image, const QueueType queueType)
{
	if (queueType == image.getOwningQueueType())
		return;

	vk::ImageMemoryBarrier releaseBarrier{};
	releaseBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(image.getOwningQueueType()));
	releaseBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	releaseBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	releaseBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
	releaseBarrier.setImage(image.getImage());

	vk::ImageMemoryBarrier aquireBarrier{};
	aquireBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(image.getOwningQueueType()));
	aquireBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	aquireBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	releaseBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
	aquireBarrier.setImage(image.getImage());

	mImageMemoryBarriers.push_back({image.getOwningQueueType(), releaseBarrier});
	mImageMemoryBarriers.push_back({ queueType, aquireBarrier });

	image.setOwningQueueType(queueType);
}


void BarrierRecorder::transferResourceToQueue(Buffer& buffer, const QueueType queueType)
{
	if (queueType == buffer.getOwningQueueType())
		return;

	vk::BufferMemoryBarrier releaseBarrier{};
	releaseBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(buffer.getOwningQueueType()));
	releaseBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	releaseBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	releaseBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
	releaseBarrier.setBuffer(buffer.getBuffer());

	vk::BufferMemoryBarrier aquireBarrier{};
	aquireBarrier.setSrcQueueFamilyIndex(getDevice()->getQueueFamilyIndex(buffer.getOwningQueueType()));
	aquireBarrier.setDstQueueFamilyIndex(getDevice()->getQueueFamilyIndex(queueType));
	aquireBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	releaseBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
	aquireBarrier.setBuffer(buffer.getBuffer());

	mBufferMemoryBarriers.push_back({ buffer.getOwningQueueType(), releaseBarrier });
	mBufferMemoryBarriers.push_back({ queueType, aquireBarrier });

	buffer.setOwningQueueType(queueType);
}


void BarrierRecorder::transitionImageLayout(Image& image, const vk::ImageLayout layout)
{
    for(uint32_t arrayLevel = 0; arrayLevel < image.numberOfLevels(); ++arrayLevel)
    {
        for(uint32_t mipLevel = 0; mipLevel < image.numberOfMips(); ++mipLevel)
        {
            vk::ImageMemoryBarrier barrier{};
            barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
            barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
            barrier.setOldLayout(image.getLayout(arrayLevel, mipLevel));
            barrier.setNewLayout(layout);
            barrier.setImage(image.getImage());
			if(layout == vk::ImageLayout::eDepthStencilAttachmentOptimal ||
			   layout == vk::ImageLayout::eDepthStencilReadOnlyOptimal)
			{
				barrier.setSubresourceRange({vk::ImageAspectFlagBits::eDepth, mipLevel, 1, arrayLevel, 1});
			}
			else
			{
				barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, mipLevel, 1, arrayLevel, 1});
            }

			image.mSubResourceInfo->at((arrayLevel * image.numberOfMips()) + mipLevel).mLayout = layout;

            mImageMemoryBarriers.push_back({image.getOwningQueueType(), barrier});
        }
    }
}


void BarrierRecorder::transitionImageLayout(ImageView& imageView, const vk::ImageLayout layout)
{
    vk::ImageMemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
	barrier.setOldLayout(imageView.getImageLayout(imageView.getBaseLevel(), imageView.getBaseMip()));
    barrier.setNewLayout(layout);
    barrier.setImage(imageView.getImage());
	if(layout == vk::ImageLayout::eDepthStencilAttachmentOptimal ||
		layout == vk::ImageLayout::eDepthStencilReadOnlyOptimal)
	{
		barrier.setSubresourceRange({vk::ImageAspectFlagBits::eDepth, imageView.getBaseMip(), imageView.getMipsCount(), imageView.getBaseLevel(), imageView.getLevelCount()});
	}
	else
	{
		barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, imageView.getBaseMip(), imageView.getMipsCount(), imageView.getBaseLevel(), imageView.getLevelCount()});
    }

    mImageMemoryBarriers.push_back({imageView.getOwningQueueType(), barrier});

	for(uint32_t arrayLevel = imageView.getBaseLevel(); arrayLevel < imageView.getBaseLevel() + imageView.getLevelCount(); ++arrayLevel)
	{
		for(uint32_t mipLevel = imageView.getBaseMip(); mipLevel < imageView.getBaseMip() + imageView.getMipsCount(); ++mipLevel)
		{
			imageView.mSubResourceInfo[(arrayLevel * imageView.mTotalMips) + mipLevel].mLayout = layout;
		}
	}
}


void BarrierRecorder::makeContentsVisibleToCompute(const Image& img)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));

	makeContentsVisible(img);
}


void BarrierRecorder::makeContentsVisibleToCompute(const ImageView& view)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));

	makeContentsVisible(view);
}


void BarrierRecorder::makeContentsVisibleToCompute(const Buffer& buf)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));

	vk::AccessFlags dstAccess = vk::AccessFlagBits::eMemoryRead;

	if (buf.getUsage() & BufferUsage::IndirectArgs)
		dstAccess |= vk::AccessFlagBits::eIndirectCommandRead;

	vk::BufferMemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	barrier.setDstAccessMask(dstAccess);
	barrier.setBuffer(buf.getBuffer());

	mBufferMemoryBarriers.push_back({ buf.getOwningQueueType(), barrier });
}


void BarrierRecorder::makeContentsVisibleToCompute(const BufferView& view)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));

	vk::AccessFlags dstAccess = vk::AccessFlagBits::eMemoryRead;

	if (view.getUsage() & BufferUsage::IndirectArgs)
		dstAccess |= vk::AccessFlagBits::eIndirectCommandRead;

	vk::BufferMemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	barrier.setDstAccessMask(dstAccess);
	barrier.setBuffer(view.getBuffer());

	mBufferMemoryBarriers.push_back({ QueueType::Graphics, barrier });
}


void BarrierRecorder::makeContentsVisibleToGraphics(const Image& img)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eColorAttachmentOutput)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eVertexShader)));

	makeContentsVisible(img);
}


void BarrierRecorder::makeContentsVisibleToGraphics(const ImageView& view)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eColorAttachmentOutput)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eVertexShader)));

	makeContentsVisible(view);
}


void BarrierRecorder::makeContentsVisibleToGraphics(const Buffer& buf)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eVertexShader)));

	vk::AccessFlags dstAccess = vk::AccessFlagBits::eMemoryRead;

	if (buf.getUsage() & BufferUsage::IndirectArgs)
		dstAccess |= vk::AccessFlagBits::eIndirectCommandRead;

	vk::BufferMemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	barrier.setDstAccessMask(dstAccess);
	barrier.setBuffer(buf.getBuffer());

	mBufferMemoryBarriers.push_back({ buf.getOwningQueueType(), barrier });
}


void BarrierRecorder::makeContentsVisibleToGraphics(const BufferView& view)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eVertexShader)));

	vk::AccessFlags dstAccess = vk::AccessFlagBits::eMemoryRead;

	if (view.getUsage() & BufferUsage::IndirectArgs)
		dstAccess |= vk::AccessFlagBits::eIndirectCommandRead;

	vk::BufferMemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	barrier.setDstAccessMask(dstAccess);
	barrier.setBuffer(view.getBuffer());

	mBufferMemoryBarriers.push_back({ QueueType::Graphics, barrier });
}


void BarrierRecorder::makeContentsVisibleFromTransfer(const Image& img)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eTransfer)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eVertexShader)));

	const auto layout = img.getLayout(0, 0);

	vk::ImageMemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
	barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
	barrier.setOldLayout(layout);
	barrier.setNewLayout(layout);
	if (layout == vk::ImageLayout::eDepthStencilAttachmentOptimal ||
		layout == vk::ImageLayout::eDepthStencilReadOnlyOptimal)
	{
		barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, 0, img.numberOfMips(), 0, img.numberOfLevels() });
	}
	else
	{
		barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, img.numberOfMips(), 0, img.numberOfLevels() });
	}

	mImageMemoryBarriers.push_back({ img.getOwningQueueType(), barrier });
}


void BarrierRecorder::makeContentsVisibleFromTransfer(const ImageView& view)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eTransfer)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eVertexShader)));

	const auto layout = view.getImageLayout();

	vk::ImageMemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
	barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
	barrier.setOldLayout(layout);
	barrier.setNewLayout(layout);
	if (layout == vk::ImageLayout::eDepthStencilAttachmentOptimal ||
		layout == vk::ImageLayout::eDepthStencilReadOnlyOptimal)
	{
		barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, view.getBaseMip(), view.getMipsCount(), view.getBaseLevel(), view.getLevelCount() });
	}
	else
	{
		barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, view.getBaseMip(), view.getMipsCount(), view.getBaseLevel(), view.getLevelCount() });
	}

	mImageMemoryBarriers.push_back({ view.getOwningQueueType(), barrier });
}


void BarrierRecorder::makeContentsVisibleFromTransfer(const Buffer& buf)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eTransfer)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eVertexInput)));

	vk::AccessFlags dstAccess = vk::AccessFlagBits::eMemoryRead;

	if (buf.getUsage() & BufferUsage::IndirectArgs)
		dstAccess |= vk::AccessFlagBits::eIndirectCommandRead;

	vk::BufferMemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	barrier.setDstAccessMask(dstAccess);
	barrier.setBuffer(buf.getBuffer());

	mBufferMemoryBarriers.push_back({ buf.getOwningQueueType(), barrier });
}


void BarrierRecorder::makeContentsVisibleFromTransfer(const BufferView& view)
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eTransfer)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eVertexInput)));

	vk::AccessFlags dstAccess = vk::AccessFlagBits::eMemoryRead;

	if (view.getUsage() & BufferUsage::IndirectArgs)
		dstAccess |= vk::AccessFlagBits::eIndirectCommandRead;

	vk::BufferMemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
	barrier.setDstAccessMask(dstAccess);
	barrier.setBuffer(view.getBuffer());

	mBufferMemoryBarriers.push_back({ QueueType::Graphics, barrier });
}


void BarrierRecorder::memoryBarrierComputeToCompute()
{
	mSrc = static_cast<vk::PipelineStageFlags>(std::min(static_cast<uint32_t>(mSrc), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));
	mDst = static_cast<vk::PipelineStageFlags>(std::max(static_cast<uint32_t>(mDst), static_cast<uint32_t>(vk::PipelineStageFlagBits::eComputeShader)));

	vk::MemoryBarrier barrier{};
	barrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
	barrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eIndirectCommandRead);

	mMemoryBarriers.push_back({ QueueType::Graphics, barrier });
}


void BarrierRecorder::makeContentsVisible(const Image& image)
{

	for(uint32_t arrayLevel = 0; arrayLevel < image.numberOfLevels(); ++arrayLevel)
	{
		for(uint32_t mipLevel = 0; mipLevel < image.numberOfMips(); ++mipLevel)
		{
			const vk::ImageLayout layout = image.getLayout(arrayLevel, mipLevel);

			const vk::AccessFlags srcAccess = [layout]()
			{
				switch(layout)
				{
					case vk::ImageLayout::eGeneral:
						return vk::AccessFlagBits::eShaderWrite;

					case vk::ImageLayout::eColorAttachmentOptimal:
						return vk::AccessFlagBits::eColorAttachmentWrite;


					default:
						return vk::AccessFlagBits::eMemoryWrite;
				}
			}();

			const vk::AccessFlags dstAccess = [layout]()
			{
				switch(layout)
				{
					case vk::ImageLayout::eGeneral:
						return vk::AccessFlagBits::eShaderRead;

					case vk::ImageLayout::eColorAttachmentOptimal:
						return vk::AccessFlagBits::eColorAttachmentRead;


					default:
						return vk::AccessFlagBits::eMemoryRead;
				}
			}();

			vk::ImageMemoryBarrier barrier{};
			barrier.setSrcAccessMask(srcAccess);
			barrier.setDstAccessMask(dstAccess);
			barrier.setOldLayout(layout);
			barrier.setNewLayout(layout);
			if(image.getLayout(arrayLevel, mipLevel) == vk::ImageLayout::eDepthStencilAttachmentOptimal ||
				image.getLayout(arrayLevel, mipLevel) == vk::ImageLayout::eDepthStencilReadOnlyOptimal)
			{
				barrier.setSubresourceRange({vk::ImageAspectFlagBits::eDepth, mipLevel, 1, arrayLevel, 1});
			}
			else
			{
				barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, mipLevel, 1, arrayLevel, 1});
			}

			mImageMemoryBarriers.push_back({image.getOwningQueueType(), barrier});
		}
	}
}


void BarrierRecorder::makeContentsVisible(const ImageView& imageView)
{	
	const vk::ImageLayout layout = imageView.getImageLayout();

	const vk::AccessFlags srcAccess = [layout]()
	{
		switch (layout)
		{
		case vk::ImageLayout::eGeneral:
			return vk::AccessFlagBits::eShaderWrite;

		case vk::ImageLayout::eColorAttachmentOptimal:
			return vk::AccessFlagBits::eColorAttachmentWrite;


		default:
			return vk::AccessFlagBits::eMemoryWrite;
		}
	}();

	const vk::AccessFlags dstAccess = [layout]()
	{
		switch (layout)
		{
		case vk::ImageLayout::eGeneral:
			return vk::AccessFlagBits::eShaderRead;

		case vk::ImageLayout::eColorAttachmentOptimal:
			return vk::AccessFlagBits::eColorAttachmentRead;


		default:
			return vk::AccessFlagBits::eMemoryRead;
		}
	}();

    vk::ImageMemoryBarrier barrier{};
	barrier.setSrcAccessMask(srcAccess);
	barrier.setDstAccessMask(dstAccess);
	barrier.setOldLayout(layout);
	barrier.setNewLayout(layout);
	if(imageView.getImageLayout() == vk::ImageLayout::eDepthStencilAttachmentOptimal ||
		imageView.getImageLayout() == vk::ImageLayout::eDepthStencilReadOnlyOptimal)
	{
		barrier.setSubresourceRange({vk::ImageAspectFlagBits::eDepth, imageView.getBaseMip(), imageView.getMipsCount(), imageView.getBaseLevel(), imageView.getLevelCount()});
	}
	else
	{
		barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, imageView.getBaseMip(), imageView.getMipsCount(), imageView.getBaseLevel(), imageView.getLevelCount()});
    }

    mImageMemoryBarriers.push_back({imageView.getOwningQueueType(), barrier});
}


std::vector<vk::ImageMemoryBarrier> BarrierRecorder::getImageBarriers(QueueType type)
{
	std::vector<vk::ImageMemoryBarrier> barriers;

	for (auto[queueType, barrier] : mImageMemoryBarriers)
	{
		if (queueType == type)
			barriers.push_back(barrier);
	}

	return barriers;
}


std::vector<vk::BufferMemoryBarrier> BarrierRecorder::getBufferBarriers(QueueType type)
{
	std::vector<vk::BufferMemoryBarrier> barriers;

	for (auto[queueType, barrier] : mBufferMemoryBarriers)
	{
		if (queueType == type)
			barriers.push_back(barrier);
	}

	return barriers;
}
