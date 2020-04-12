#ifndef CONVERSIONUTILS_HPP
#define CONVERSIONUTILS_HPP

#include "Engine/PassTypes.hpp"
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

#endif