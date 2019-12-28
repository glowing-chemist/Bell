#include "Core/Image.hpp"
#include "Core/BellLogging.hpp"
#include "RenderDevice.hpp"
#include "Core/ConversionUtils.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanImage.hpp"
#endif

ImageBase::ImageBase(RenderDevice* dev,
			 const Format format,
			 const ImageUsage usage,
             const uint32_t x,
             const uint32_t y,
             const uint32_t z,
             const uint32_t mips,
             const uint32_t levels,
             const uint32_t samples,
			 const std::string& debugName) :
	GPUResource{dev->getCurrentSubmissionIndex()},
	DeviceChild{dev},
    mNumberOfMips{mips},
    mNumberOfLevels{levels},
    mIsOwned{true},
    mFormat{format},
    mUsage{usage},
    mSamples{samples},
    mDebugName{debugName}
{
	BELL_ASSERT(x > 0 && y > 0 && z > 0, "Image must have non zero dimensions")
	BELL_ASSERT(mips > 0 && levels > 0, "Image must have non zero mips and/or levels")

	mSubResourceInfo = new std::vector<SubResourceInfo>{};

    for(uint32_t arrayLevel = 0; arrayLevel < mNumberOfLevels; ++arrayLevel)
    {
        ImageExtent extent{x, y, z};

        for(uint32_t mipsLevel = 0; mipsLevel < mNumberOfMips; ++mipsLevel)
        {
            SubResourceInfo subInfo{};
            subInfo.mLayout = ImageLayout::Undefined;
            subInfo.mExtent = extent;

            extent = ImageExtent{extent.width / 2, extent.height / 2, extent.depth / 2};

			mSubResourceInfo->push_back(subInfo);
        }
    }
}


ImageBase::~ImageBase()
{
	delete mSubResourceInfo;
}


void ImageBase::swap(ImageBase& other)
{
    const bool isOwned = mIsOwned;
	const Format Format = mFormat;
	const ImageUsage Usage = mUsage;
    const uint32_t NumberOfMips = mNumberOfMips;
    const uint32_t NumberOfLevels = mNumberOfLevels;
    const std::string DebugName = mDebugName;

	mIsOwned = other.mIsOwned;
	other.mIsOwned = isOwned;

	mFormat = other.mFormat;
	other.mFormat = Format;

	mUsage = other.mUsage;
	other.mUsage = Usage;

	std::vector<SubResourceInfo>* temp;
	temp = mSubResourceInfo;
	mSubResourceInfo = other.mSubResourceInfo;
	other.mSubResourceInfo = temp;

	mNumberOfMips = other.mNumberOfMips;
	other.mNumberOfMips = NumberOfMips;

    mNumberOfLevels = other.mNumberOfLevels;
    other.mNumberOfLevels = NumberOfLevels;

	mDebugName = other.mDebugName;
	other.mDebugName = DebugName;
}


void ImageBase::updateLastAccessed()
{
	GPUResource::updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
}


Image::Image(RenderDevice* dev,
	const Format format,
	const ImageUsage usage,
	const uint32_t x,
	const uint32_t y,
	const uint32_t z,
	const uint32_t mips,
	const uint32_t levels,
	const uint32_t samples,
	const std::string& name)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanImage>(dev, format, usage, x, y, z, mips, levels, samples, name);
#endif
}
