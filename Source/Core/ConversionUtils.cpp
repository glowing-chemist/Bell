#include "ConversionUtils.hpp"



vk::ImageLayout getVulkanImageLayout(const AttachmentType type)
{
	switch(type)
	{
		case AttachmentType::RenderTarget1D:
		case AttachmentType::RenderTarget2D:
		case AttachmentType::RenderTarget3D:
			return vk::ImageLayout::eColorAttachmentOptimal;

		case AttachmentType::Image1D:
		case AttachmentType::Image2D:
		case AttachmentType::Image3D:
			return vk::ImageLayout::eGeneral;

		case AttachmentType::Texture1D:
		case AttachmentType::Texture2D:
		case AttachmentType::Texture3D:
			return vk::ImageLayout::eShaderReadOnlyOptimal;

		case AttachmentType::Depth:
			return vk::ImageLayout::eDepthStencilAttachmentOptimal;

		default:
			return vk::ImageLayout::eColorAttachmentOptimal;
	}

}


vk::Format getVulkanImageFormat(const Format format)
{
	switch(format)
	{
		case Format::RGBA8SRGB:
			return vk::Format::eR8G8B8A8Srgb;

		case Format::RGB32SFloat:
			return vk::Format::eR32G32B32Sfloat;

		case Format::RGBA32SFloat:
			return vk::Format::eR32G32B32A32Sfloat;

		case Format::RGBA8UNorm:
			return vk::Format::eR8G8B8A8Unorm;

		case Format::BGRA8UNorm:
            return vk::Format::eB8G8R8A8Unorm;

		case Format::D32Float:
			return vk::Format::eD32Sfloat;

		case Format::D24S8Float:
			return vk::Format::eD32SfloatS8Uint;

		case Format::R32Uint:
			return vk::Format::eR32Uint;

		case Format::R8UNorm:
			return vk::Format::eR8Unorm;

        case Format::RG8UNorm:
            return vk::Format::eR8G8Unorm;

		case Format::RG16UNorm:
			return vk::Format::eR16G16Unorm;

		case Format::RGB16UNorm:
			return vk::Format::eR16G16B16Unorm;

		case Format::RGBA16UNorm:
			return vk::Format::eR16G16B16A16Unorm;

		case Format::RGBA16SNorm:
			return vk::Format::eR16G16B16A16Snorm;

		case Format::RGBA32Float:
			return vk::Format::eR32G32B32A32Sfloat;

        case Format::RGB8UNorm:
            return vk::Format::eR8G8B8Unorm;

        case Format::RGB8SRGB:
            return vk::Format::eR8G8B8Srgb;

		case Format::RG32Float:
			return vk::Format::eR32G32Sfloat;

		case Format::RGBA16UInt:
			return vk::Format::eR16G16B16A16Uint;

		case Format::R16UInt:
			return vk::Format::eR16Uint;

		default:
			return vk::Format::eR8G8B8A8Srgb;
	}
}


vk::ImageUsageFlags getVulkanImageUsage(const ImageUsage usage)
{
	vk::ImageUsageFlags vulkanFlags = static_cast<vk::ImageUsageFlags>(0);

	if(usage & ImageUsage::Sampled)
		vulkanFlags |= vk::ImageUsageFlagBits::eSampled;

	if(usage & ImageUsage::ColourAttachment)
		vulkanFlags |= vk::ImageUsageFlagBits::eColorAttachment;

	if(usage & ImageUsage::DepthStencil)
		vulkanFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

	if(usage & ImageUsage::Storage)
		vulkanFlags |= vk::ImageUsageFlagBits::eStorage;

	if(usage & ImageUsage::TransferDest)
		vulkanFlags |= vk::ImageUsageFlagBits::eTransferDst;

	if(usage & ImageUsage::TransferSrc)
		vulkanFlags |= vk::ImageUsageFlagBits::eTransferSrc;


	return vulkanFlags;
}


vk::BufferUsageFlags getVulkanBufferUsage(const BufferUsage usage)
{
	vk::BufferUsageFlags vulkanFlags = static_cast<vk::BufferUsageFlags>(0);

	if (usage & BufferUsage::Vertex)
		vulkanFlags |= vk::BufferUsageFlagBits::eVertexBuffer;

	if (usage & BufferUsage::Index)
		vulkanFlags |= vk::BufferUsageFlagBits::eIndexBuffer;

	if (usage & BufferUsage::Uniform)
		vulkanFlags |= vk::BufferUsageFlagBits::eUniformBuffer;

	if (usage & BufferUsage::DataBuffer)
		vulkanFlags |= vk::BufferUsageFlagBits::eStorageBuffer;

	if (usage & BufferUsage::IndirectArgs)
		vulkanFlags |= vk::BufferUsageFlagBits::eIndirectBuffer;

	if (usage & BufferUsage::TransferSrc)
		vulkanFlags |= vk::BufferUsageFlagBits::eTransferSrc;

	if (usage & BufferUsage::TransferDest)
		vulkanFlags |= vk::BufferUsageFlagBits::eTransferDst;


	return vulkanFlags;
}


uint32_t getPixelSize(const Format format)
{
	uint32_t result = 4;

	switch(format)
	{
	case Format::RGBA8UNorm:
		result = 4;
		break;

	case Format::RGB32SFloat:
		result = 16;
		break;

	default:
		result =  4;
	}

	return result;
}


Format getBellImageFormat(const vk::Format format)
{
	switch(format)
	{
		case vk::Format::eR8G8B8A8Srgb:
			return Format::RGBA8SRGB;

		case vk::Format::eR8G8B8A8Unorm:
			return Format::RGBA8UNorm;

		case vk::Format::eB8G8R8A8Unorm:
			return Format::BGRA8UNorm;

		case vk::Format::eD32Sfloat:
			return Format::D32Float;

		case vk::Format::eD32SfloatS8Uint:
			return Format::D24S8Float;

		default:
			return Format::RGBA8SRGB;
	}
}

