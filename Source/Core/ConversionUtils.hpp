#ifndef CONVERSIONUTILS_HPP
#define CONVERSIONUTILS_HPP

#include "Engine/PassTypes.hpp"


#include <vulkan/vulkan.hpp>


vk::ImageLayout getVulkanImageLayout(const AttachmentType);

vk::Format getVulkanImageFormat(const Format);

vk::ImageUsageFlags getVulkanImageUsage(const ImageUsage);

vk::BufferUsageFlags getVulkanBufferUsage(const BufferUsage);

uint32_t getPixelSize(const Format);

Format getBellImageFormat(const vk::Format);

#endif
