#ifndef CONVERSIONUTILS_HPP
#define CONVERSIONUTILS_HPP

#include "Engine/PassTypes.hpp"
#include "Core/GPUResource.hpp"
#include "Core/BarrierManager.hpp"

#include <vulkan/vulkan.hpp>


vk::ImageLayout getVulkanImageLayout(const AttachmentType);

ImageLayout getImageLayout(const AttachmentType);

AttachmentType getAttachmentType(const vk::ImageLayout);

AttachmentType getAttachmentType(const ImageLayout);

vk::Format getVulkanImageFormat(const Format);

vk::ImageUsageFlags getVulkanImageUsage(const ImageUsage);

vk::BufferUsageFlags getVulkanBufferUsage(const BufferUsage);

uint32_t getPixelSize(const Format);

Format getBellImageFormat(const vk::Format);

vk::ImageLayout getVulkanImageLayout(const ImageLayout);

vk::PipelineStageFlags getVulkanPipelineStage(const SyncPoint);

SyncPoint getSyncPoint(const AttachmentType);

#endif
