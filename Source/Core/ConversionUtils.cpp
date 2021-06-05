#include "ConversionUtils.hpp"

#ifdef VULKAN

vk::ImageLayout getVulkanImageLayout(const AttachmentType type)
{
	switch(type)
	{
		case AttachmentType::RenderTarget1D:
		case AttachmentType::RenderTarget2D:
			return vk::ImageLayout::eColorAttachmentOptimal;

		case AttachmentType::Image1D:
		case AttachmentType::Image2D:
		case AttachmentType::Image3D:
			return vk::ImageLayout::eGeneral;

		case AttachmentType::Texture1D:
		case AttachmentType::Texture2D:
		case AttachmentType::Texture3D:
        case AttachmentType::CubeMap:
			return vk::ImageLayout::eShaderReadOnlyOptimal;

		case AttachmentType::Depth:
			return vk::ImageLayout::eDepthStencilAttachmentOptimal;

		case AttachmentType::TransferDestination:
			return vk::ImageLayout::eTransferDstOptimal;

		case AttachmentType::TransferSource:
			return vk::ImageLayout::eTransferSrcOptimal;

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

	case vk::ImageLayout::eTransferDstOptimal:
		return AttachmentType::TransferDestination;

	case vk::ImageLayout::eTransferSrcOptimal:
		return AttachmentType::TransferSource;

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

	case Format::R32Float:
		return vk::Format::eR32Sfloat;

	case Format::R8UNorm:
		return vk::Format::eR8Unorm;

    case Format::R8Norm:
        return vk::Format::eR8Snorm;

	case Format::RG8UNorm:
		return vk::Format::eR8G8Unorm;

	case Format::RG16UNorm:
		return vk::Format::eR16G16Unorm;

	case Format::RG16SNorm:
	    return vk::Format::eR16G16Snorm;

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

    case Format::RG16Float:
        return vk::Format::eR16G16Sfloat;

	case Format::RGBA16UInt:
		return vk::Format::eR16G16B16A16Uint;

    case Format::RGBA16Int:
        return vk::Format::eR16G16B16A16Sint;

    case Format::RGBA8Uint:
        return vk::Format::eR8G8B8A8Uint;

	case Format::R16UInt:
		return vk::Format::eR16Uint;

    case Format::RGBA16Float:
        return vk::Format::eR16G16B16A16Sfloat;

	default:
        BELL_TRAP;
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

    if(usage & BufferUsage::CommandPredication)
        vulkanFlags |= vk::BufferUsageFlagBits::eConditionalRenderingEXT;

    if(usage & BufferUsage::AccelerationStructure)
        vulkanFlags |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;

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
        BELL_TRAP;
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
    vk::PipelineStageFlags flags = vk::PipelineStageFlags{};

    if(syncPoint & SyncPoint::TopOfPipe)
        flags |= vk::PipelineStageFlagBits::eTopOfPipe;

    if(syncPoint & SyncPoint::IndirectArgs)
        flags |= vk::PipelineStageFlagBits::eDrawIndirect;

    if(syncPoint & SyncPoint::CommandPredication)
        flags |= vk::PipelineStageFlagBits::eConditionalRenderingEXT;

    if(syncPoint & SyncPoint::TransferSource)
        flags |= vk::PipelineStageFlagBits::eTransfer;

    if(syncPoint & SyncPoint::TransferDestination)
        flags |= vk::PipelineStageFlagBits::eTransfer;

    if(syncPoint & SyncPoint::VertexInput)
        flags |= vk::PipelineStageFlagBits::eVertexInput;

    if(syncPoint & SyncPoint::VertexShader)
        flags |= vk::PipelineStageFlagBits::eVertexShader;

    if(syncPoint & SyncPoint::FragmentShader)
        flags |= vk::PipelineStageFlagBits::eFragmentShader;

    if(syncPoint & SyncPoint::FragmentShaderOutput)
        flags |= vk::PipelineStageFlagBits::eColorAttachmentOutput;

    if(syncPoint & SyncPoint::ComputeShader)
        flags |= vk::PipelineStageFlagBits::eComputeShader;

    if(syncPoint & SyncPoint::BottomOfPipe)
        flags |= vk::PipelineStageFlagBits::eBottomOfPipe;

    return flags;
}
#endif

#ifdef DX_12

D3D12_RESOURCE_STATES getDX12ImageLayout(const AttachmentType type)
{
	switch (type)
	{
	case AttachmentType::RenderTarget1D:
	case AttachmentType::RenderTarget2D:
		return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;

	case AttachmentType::Image1D:
	case AttachmentType::Image2D:
	case AttachmentType::Image3D:
		return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	case AttachmentType::Texture1D:
	case AttachmentType::Texture2D:
	case AttachmentType::Texture3D:
		return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	case AttachmentType::Depth:
		return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;

	default:
		return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}
}


// This conversion looses dimensionality.
AttachmentType getAttachmentType(const D3D12_RESOURCE_STATES)
{
	__debugbreak();
	return AttachmentType::Texture2D;
}


DXGI_FORMAT getDX12ImageFormat(const Format format)
{
	switch (format)
	{
	case Format::RGBA8SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	case Format::RGB32SFloat:
		return DXGI_FORMAT_R32G32B32_FLOAT;

	case Format::RGBA32SFloat:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;

	case Format::RGBA8UNorm:
		return DXGI_FORMAT_R8G8B8A8_UNORM;

	case Format::BGRA8UNorm:
		return DXGI_FORMAT_B8G8R8A8_UNORM;

	case Format::D32Float:
		return DXGI_FORMAT_D32_FLOAT;

	case Format::D24S8Float:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	case Format::R32Uint:
		return DXGI_FORMAT_R32_UINT;

	case Format::R32Float:
		return DXGI_FORMAT_R32_FLOAT;

	case Format::R8UNorm:
		return DXGI_FORMAT_R8_UNORM;

	case Format::RG8UNorm:
		return DXGI_FORMAT_R8G8_UNORM;

	case Format::RG16UNorm:
		return DXGI_FORMAT_R16G16_UNORM;

	case Format::RGB16UNorm:
		return DXGI_FORMAT_R16G16B16A16_UNORM;

	case Format::RGBA16UNorm:
		return DXGI_FORMAT_R16G16B16A16_UNORM;

	case Format::RGBA16SNorm:
		return DXGI_FORMAT_R16G16B16A16_SNORM;

	case Format::RGBA32Float:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;

	case Format::RGB8UNorm:
		return DXGI_FORMAT_R8G8B8A8_UNORM;

	case Format::RGB8SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	case Format::RG32Float:
		return DXGI_FORMAT_R32G32_FLOAT;

	case Format::RGBA16UInt:
		return DXGI_FORMAT_R16G16B16A16_UINT;

	case Format::R16UInt:
		return DXGI_FORMAT_R16_UINT;

	default:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}
}


D3D12_RESOURCE_FLAGS getDX12ImageUsage(const ImageUsage usage)
{
	D3D12_RESOURCE_FLAGS usageFlags = D3D12_RESOURCE_FLAG_NONE;

	if (!(usage & ImageUsage::Sampled))
		usageFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

	if (usage & ImageUsage::ColourAttachment)
		usageFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	if (usage & ImageUsage::DepthStencil)
		usageFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	if (usage & ImageUsage::Storage)
		usageFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;


	return usageFlags;
}


D3D12_RESOURCE_STATES getDX12ImageLayout(const ImageLayout layout)
{
	switch (layout)
	{
	case ImageLayout::ColorAttachment:
		return D3D12_RESOURCE_STATE_RENDER_TARGET;

	case ImageLayout::DepthStencil:
		return D3D12_RESOURCE_STATE_DEPTH_WRITE;

	case ImageLayout::DepthStencilRO:
		return D3D12_RESOURCE_STATE_DEPTH_READ;

	case ImageLayout::General:
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	case ImageLayout::Present:
		return D3D12_RESOURCE_STATE_PRESENT;

	case ImageLayout::Sampled:
		return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	case ImageLayout::TransferDst:
		return D3D12_RESOURCE_STATE_COPY_DEST;

	case ImageLayout::TransferSrc:
		return D3D12_RESOURCE_STATE_COPY_SOURCE;

	case ImageLayout::Undefined:
		return D3D12_RESOURCE_STATE_COMMON;
	}
}

uint32_t getDX12PipelineStage(const SyncPoint)
{
	/*switch (syncPoint)
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

	return vk::PipelineStageFlagBits::eTopOfPipe;*/
	return 0;
}

#endif

ImageLayout getImageLayout(const AttachmentType type)
{
	switch (type)
	{
	case AttachmentType::RenderTarget1D:
	case AttachmentType::RenderTarget2D:
		return ImageLayout::ColorAttachment;

	case AttachmentType::Image1D:
	case AttachmentType::Image2D:
	case AttachmentType::Image3D:
		return ImageLayout::General;

	case AttachmentType::Texture1D:
	case AttachmentType::Texture2D:
	case AttachmentType::Texture3D:
    case AttachmentType::CubeMap:
		return  ImageLayout::Sampled;

	case AttachmentType::Depth:
		return ImageLayout::DepthStencil;

	case AttachmentType::TransferDestination:
		return ImageLayout::TransferDst;

	case AttachmentType::TransferSource:
		return ImageLayout::TransferSrc;

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

	case ImageLayout::TransferDst:
		return AttachmentType::TransferDestination;

	case ImageLayout::TransferSrc:
		return AttachmentType::TransferSource;

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
        result = 12;
		break;

    case Format::RGBA32Float:
    case Format::RGBA32SFloat:
        result = 16;
        break;

    case Format::RG32Float:
        result = 8;
        break;

    case Format::RGBA16Int:
        result = 8;
        break;

	case Format::R8UNorm:
        result = 1;
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
		return SyncPoint::FragmentShaderOutput;

	case AttachmentType::Image1D:
	case AttachmentType::Image2D:
	case AttachmentType::Image3D:
		return SyncPoint::FragmentShader;

	case AttachmentType::Texture1D:
	case AttachmentType::Texture2D:
	case AttachmentType::Texture3D:
    case AttachmentType::CubeMap:
		return  SyncPoint::VertexShader;

	case AttachmentType::DataBufferRO:
	case AttachmentType::DataBufferWO:
	case AttachmentType::DataBufferRW:
		return SyncPoint::TopOfPipe;

	case AttachmentType::Depth:
		return SyncPoint::FragmentShaderOutput;

    case AttachmentType::CommandPredicationBuffer:
        return SyncPoint::CommandPredication;

	case AttachmentType::TransferDestination:
		return SyncPoint::TransferDestination;

	case AttachmentType::TransferSource:
		return SyncPoint::TransferSource;

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
		return "Render Target";

	case AttachmentType::Image1D:
	case AttachmentType::Image2D:
	case AttachmentType::Image3D:
		return "Storage Image";

	case AttachmentType::Texture1D:
	case AttachmentType::Texture2D:
	case AttachmentType::Texture3D:
    case AttachmentType::CubeMap:
		return  "Sampled Image";

	case AttachmentType::Depth:
		return "Depth Stencil";

	case AttachmentType::DataBufferRO:
	case AttachmentType::DataBufferWO:
	case AttachmentType::DataBufferRW:
		return "Data Buffer";

	case AttachmentType::UniformBuffer:
		return "Uniform Buffer";

	case AttachmentType::TransferDestination:
		return "Transfer destination";

	case AttachmentType::TransferSource:
		return "Transfer source";

	case AttachmentType::CommandPredicationBuffer:
		return "Predication buffer";

	case AttachmentType::VertexBuffer:
		return "Vertex buffer";

	case AttachmentType::IndexBuffer:
		return "Index buffer";

    case AttachmentType::Sampler:
        return "Sampler";

    case AttachmentType::IndirectBuffer:
        return "Indirect buffer";

	default:
		return "Add new conversion!!!!";
	}
}
