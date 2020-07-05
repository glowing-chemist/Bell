#ifndef CONVERSIONUTILS_HPP
#define CONVERSIONUTILS_HPP

#include "Engine/PassTypes.hpp"
#include "Engine/GeomUtils.h"
#include "Core/GPUResource.hpp"
#include "Core/BarrierManager.hpp"


#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#endif

#ifdef DX_12
#include <d3d12.h>
#undef max
#undef min
#endif

#ifdef VULKAN
vk::ImageLayout getVulkanImageLayout(const AttachmentType);

AttachmentType getAttachmentType(const vk::ImageLayout);

vk::Format getVulkanImageFormat(const Format);

vk::ImageUsageFlags getVulkanImageUsage(const ImageUsage);

vk::BufferUsageFlags getVulkanBufferUsage(const BufferUsage);

Format getBellImageFormat(const vk::Format);

vk::ImageLayout getVulkanImageLayout(const ImageLayout);

vk::PipelineStageFlags getVulkanPipelineStage(const SyncPoint);

#endif

#ifdef DX_12
D3D12_RESOURCE_STATES getDX12ImageLayout(const AttachmentType);

AttachmentType getAttachmentType(const D3D12_RESOURCE_STATES);

DXGI_FORMAT getDX12ImageFormat(const Format);

D3D12_RESOURCE_FLAGS getDX12ImageUsage(const ImageUsage);

D3D12_RESOURCE_STATES getDX12ImageLayout(const ImageLayout);

uint32_t getDX12PipelineStage(const SyncPoint); // Need to investigate Syn on DX12 more, looks like barriers are immediate?
#endif

ImageLayout getImageLayout(const AttachmentType);

AttachmentType getAttachmentType(const ImageLayout);

uint32_t getPixelSize(const Format);

SyncPoint getSyncPoint(const AttachmentType);

const char* getLayoutName(const ImageLayout);

const char* getAttachmentName(const AttachmentType);

inline uint32_t packColour(const float4& colour)
{
    return uint32_t(colour.r * 255.0f) | (uint32_t(colour.g * 255.0f) << 8) | (uint32_t(colour.b * 255.0f) << 16) | (uint32_t(colour.a * 255.0f) << 24);
}

inline float4 unpackColour(const uint32_t colour)
{
    return float4{(colour & 0xFF) / 255.0f, ((colour & 0xFF00) >> 8) / 255.0f, ((colour & 0xFF0000) >> 16) / 255.0f, ((colour & 0xFF000000) >> 24) / 255.0f};
}

#endif
