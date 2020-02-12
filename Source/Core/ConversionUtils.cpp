#include "ConversionUtils.hpp"

#ifdef VULKAN

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


// This conversion looses dimensionality.
AttachmentType getAttachmentType(const vk::ImageLayout layout)
{
	switch (layout)
	{
	case vk::ImageLayout::eColorAttachmentOptimal:
		return AttachmentType::RenderTarget2D;

	case vk::ImageLayout::eGeneral:
		return AttachmentType::Image2D;

	case vk::ImageLayout::eShaderReadOnlyOptimal:
		return AttachmentType::Texture2D;

	case vk::ImageLayout::eDepthStencilAttachmentOptimal:
		return AttachmentType::Depth;

	default:
		return AttachmentType::PushConstants;// to indicate undefined layout.
	}
}



vk::Format getVulkanImageFormat(const Format format)
{
	switch (format)
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

	if (usage & ImageUsage::Sampled)
		vulkanFlags |= vk::ImageUsageFlagBits::eSampled;

	if (usage & ImageUsage::ColourAttachment)
		vulkanFlags |= vk::ImageUsageFlagBits::eColorAttachment;

	if (usage & ImageUsage::DepthStencil)
		vulkanFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

	if (usage & ImageUsage::Storage)
		vulkanFlags |= vk::ImageUsageFlagBits::eStorage;

	if (usage & ImageUsage::TransferDest)
		vulkanFlags |= vk::ImageUsageFlagBits::eTransferDst;

	if (usage & ImageUsage::TransferSrc)
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


Format getBellImageFormat(const vk::Format format)
{
	switch (format)
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

vk::ImageLayout getVulkanImageLayout(const ImageLayout layout)
{
	switch (layout)
	{
	case ImageLayout::ColorAttachment:
		return vk::ImageLayout::eColorAttachmentOptimal;

	case ImageLayout::DepthStencil:
		return vk::ImageLayout::eDepthStencilAttachmentOptimal;

	case ImageLayout::DepthStencilRO:
		return vk::ImageLayout::eDepthStencilReadOnlyOptimal;

	case ImageLayout::General:
		return vk::ImageLayout::eGeneral;

	case ImageLayout::Present:
		return vk::ImageLayout::ePresentSrcKHR;

	case ImageLayout::Sampled:
		return vk::ImageLayout::eShaderReadOnlyOptimal;

	case ImageLayout::TransferDst:
		return vk::ImageLayout::eTransferDstOptimal;

	case ImageLayout::TransferSrc:
		return vk::ImageLayout::eTransferSrcOptimal;

	case ImageLayout::Undefined:
		return vk::ImageLayout::eUndefined;
	}
}

vk::PipelineStageFlags getVulkanPipelineStage(const SyncPoint syncPoint)
{
	switch (syncPoint)
	{
	case SyncPoint::TopOfPipe:
		return vk::PipelineStageFlagBits::eTopOfPipe;

	case SyncPoint::IndirectArgs:
		return vk::PipelineStageFlagBits::eDrawIndirect;

	case SyncPoint::TransferSource:
		return vk::PipelineStageFlagBits::eTransfer;

	case SyncPoint::TransferDestination:
		return vk::PipelineStageFlagBits::eTransfer;

	case SyncPoint::VertexInput:
		return vk::PipelineStageFlagBits::eVertexInput;

	case SyncPoint::VertexShader:
		return vk::PipelineStageFlagBits::eVertexShader;

	case SyncPoint::FragmentShader:
		return vk::PipelineStageFlagBits::eFragmentShader;

	case SyncPoint::FragmentShaderOutput:
		return vk::PipelineStageFlagBits::eColorAttachmentOutput;

	case SyncPoint::ComputeShader:
		return vk::PipelineStageFlagBits::eComputeShader;

	case SyncPoint::BottomOfPipe:
		return vk::PipelineStageFlagBits::eBottomOfPipe;
	}

	return vk::PipelineStageFlagBits::eTopOfPipe;
}
#endif

#ifdef OPENGL
int getOpenGLImageFormat(const Format format)
{
	switch (format)
	{
	case Format::RGBA8SRGB:
		return GL_SRGB8_ALPHA8;

	case Format::RGB32SFloat:
		return GL_RGB32F;

	case Format::RGBA32SFloat:
		return GL_RGBA32F;

	case Format::RGBA8UNorm:
		return GL_RGBA8;

	case Format::BGRA8UNorm:
		return GL_BGRA;

	case Format::D32Float:
		return GL_DEPTH32F_STENCIL8;

	case Format::D24S8Float:
		return GL_DEPTH24_STENCIL8;

	case Format::R32Uint:
		return GL_R32I;

	case Format::R8UNorm:
		return GL_R8;

	case Format::RG8UNorm:
		return GL_RG8;

	case Format::RG16UNorm:
		return GL_RG16;

	case Format::RGB16UNorm:
		return GL_RGB16;

	case Format::RGBA16UNorm:
		return GL_RGBA16;

	case Format::RGBA16SNorm:
		return GL_RGBA16_SNORM;

	case Format::RGBA32Float:
		return GL_RGBA32F;

	case Format::RGB8UNorm:
		return GL_RGB8;

	case Format::RGB8SRGB:
		return GL_SRGB8;

	case Format::RG32Float:
		return GL_RG32F;

	case Format::RGBA16UInt:
		return GL_RGBA16UI;

	case Format::R16UInt:
		return GL_R16UI;

	default:
		return GL_SRGB8_ALPHA8;
	}
}
#endif


ImageLayout getImageLayout(const AttachmentType type)
{
	switch (type)
	{
	case AttachmentType::RenderTarget1D:
	case AttachmentType::RenderTarget2D:
	case AttachmentType::RenderTarget3D:
		return ImageLayout::ColorAttachment;

	case AttachmentType::Image1D:
	case AttachmentType::Image2D:
	case AttachmentType::Image3D:
		return ImageLayout::General;

	case AttachmentType::Texture1D:
	case AttachmentType::Texture2D:
	case AttachmentType::Texture3D:
		return  ImageLayout::Sampled;

	case AttachmentType::Depth:
		return ImageLayout::DepthStencil;

	default:
		return ImageLayout::ColorAttachment;
	}
}


AttachmentType getAttachmentType(const ImageLayout layout)
{
	switch (layout)
	{
	case ImageLayout::ColorAttachment:
		return AttachmentType::RenderTarget2D;

	case ImageLayout::General:
		return AttachmentType::Image2D;

	case ImageLayout::Sampled :
		return AttachmentType::Texture2D;

	case ImageLayout::DepthStencil:
		return AttachmentType::Depth;

	default:
		return AttachmentType::PushConstants;// to indicate undefined layout.
	}
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


SyncPoint getSyncPoint(const AttachmentType type)
{
	switch (type)
	{
	case AttachmentType::RenderTarget1D:
	case AttachmentType::RenderTarget2D:
	case AttachmentType::RenderTarget3D:
		return SyncPoint::FragmentShaderOutput;

	case AttachmentType::Image1D:
	case AttachmentType::Image2D:
	case AttachmentType::Image3D:
		return SyncPoint::FragmentShader;

	case AttachmentType::Texture1D:
	case AttachmentType::Texture2D:
	case AttachmentType::Texture3D:
		return  SyncPoint::VertexShader;

	case AttachmentType::DataBufferRO:
	case AttachmentType::DataBufferWO:
	case AttachmentType::DataBufferRW:
		return SyncPoint::TopOfPipe;

	case AttachmentType::Depth:
		return SyncPoint::FragmentShaderOutput;

	default:
		return SyncPoint::TopOfPipe;
	}
}

const char* getLayoutName(const ImageLayout layout)
{
	switch (layout)
	{
	case ImageLayout::ColorAttachment:
		return "Colour attachment";

	case ImageLayout::DepthStencil:
		return "Depth Stencil";

	case ImageLayout::DepthStencilRO:
		return "Depth Stencil RO";

	case ImageLayout::General:
		return "General";

	case ImageLayout::Present:
		return "Present";

	case ImageLayout::Sampled:
		return "Sampled";

	case ImageLayout::TransferDst:
		return "Transfer Dest";

	case ImageLayout::TransferSrc:
		return "Transfer Source";

	case ImageLayout::Undefined:
		return "Undefined";
	}
}


const char* getAttachmentName(const AttachmentType type)
{
	switch (type)
	{
	case AttachmentType::RenderTarget1D:
	case AttachmentType::RenderTarget2D:
	case AttachmentType::RenderTarget3D:
		return "Render Target";

	case AttachmentType::Image1D:
	case AttachmentType::Image2D:
	case AttachmentType::Image3D:
		return "Storage Image";

	case AttachmentType::Texture1D:
	case AttachmentType::Texture2D:
	case AttachmentType::Texture3D:
		return  "Sampled Image";

	case AttachmentType::Depth:
		return "Depth Stencil";

	case AttachmentType::DataBufferRO:
	case AttachmentType::DataBufferWO:
	case AttachmentType::DataBufferRW:
		return "Data Buffer";

	case AttachmentType::UniformBuffer:
		return "Uniform Buffer";

	default:
		return "Add new conversion!!!!";
	}
}
