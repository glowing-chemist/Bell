#include "VulkanShaderResourceSet.hpp"

#include "VulkanRenderDevice.hpp"
#include "VulkanImageView.hpp"
#include "VulkanBufferView.hpp"
#include "Core/Sampler.hpp"


VulkanShaderResourceSet::VulkanShaderResourceSet(RenderDevice* dev) :
	ShaderResourceSetBase(dev),
	mLayout{nullptr},
	mDescSet{nullptr},
	mPool{nullptr} {}


VulkanShaderResourceSet::~VulkanShaderResourceSet()
{
	getDevice()->destroyShaderResourceSet(*this);
}


void VulkanShaderResourceSet::finalise()
{
	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

	mLayout = device->generateDescriptorSetLayoutBindings(mResources);

	std::vector<WriteShaderResourceSet> writes{};
	uint32_t binding = 0;
	for (const auto& resource : mResources)
	{
		switch (resource.mType)
		{
		case AttachmentType::Texture1D:
		case AttachmentType::Texture2D:
		case AttachmentType::Texture3D:
		case AttachmentType::Image1D:
		case AttachmentType::Image2D:
		case AttachmentType::Image3D:
		{
			writes.emplace_back(resource.mType, binding, 0, &mImageViews[resource.mIndex], nullptr, nullptr);
			break;
		}

		case AttachmentType::TextureArray:
		{
			writes.emplace_back(resource.mType, binding, mImageArrays[resource.mIndex].size(), mImageArrays[resource.mIndex].data(), nullptr, nullptr);
			break;
		}

		case AttachmentType::Sampler:
		{
			writes.emplace_back(resource.mType, binding, 0, nullptr, nullptr, &mSamplers[resource.mIndex]);
			break;
		}

		case AttachmentType::UniformBuffer:
        case AttachmentType::DataBufferRO:
        case AttachmentType::DataBufferRW:
        case AttachmentType::DataBufferWO:
		{
			writes.emplace_back(resource.mType, binding, 0, nullptr, &mBufferViews[resource.mIndex], nullptr);
			break;
		}

        default:
            // THis attachment type doesn't make sense to add to a SRS.
            BELL_TRAP;
            break;

		}
		++binding;
	}

	mDescSet = device->getDescriptorManager()->writeShaderResourceSet(mLayout, writes);
}
