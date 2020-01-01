#ifndef CONVERSIONUTILS_HPP
#define CONVERSIONUTILS_HPP

#include "Engine/PassTypes.hpp"
#include "Core/GPUResource.hpp"
#include "Core/BarrierManager.hpp"


#ifdef VULKAN
#include <vulkan/vulkan.hpp>
#endif

#ifdef OPENGL
#include "glad/glad.h"
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

#ifdef OPENGL
int getOpenGLImageFormat(const Format);
#endif

ImageLayout getImageLayout(const AttachmentType);

AttachmentType getAttachmentType(const ImageLayout);

uint32_t getPixelSize(const Format);

SyncPoint getSyncPoint(const AttachmentType);

#endif
