#include "ConversionUtils.hpp"



vk::ImageLayout getVulkanImageLayout(const AttachmentType type)
{
	switch(type)
	{
		case AttachmentType::RenderTarget1D:
		case AttachmentType::RenderTarget2D:
		case AttachmentType::RenderTarget3D:
			return vk::ImageLayout::eColorAttachmentOptimal;

		case AttachmentType::Depth:
			return vk::ImageLayout::eDepthStencilAttachmentOptimal;

		case AttachmentType::SwapChain:
			return vk::ImageLayout::eColorAttachmentOptimal;

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

		case Format::RGBA8UNorm:
			return vk::Format::eR8G8B8A8Unorm;

		case Format::BGRA8UNorm:
			return vk::Format::eB8G8R8A8Unorm;

		case Format::D32Float:
			return vk::Format::eD32Sfloat;

		case Format::D24S8Float:
			return vk::Format::eD32SfloatS8Uint;

		default:
			return vk::Format::eR8G8B8A8Srgb;
	}
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

