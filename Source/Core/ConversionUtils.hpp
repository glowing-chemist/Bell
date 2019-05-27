#ifndef CONVERSIONUTILS_HPP
#define CONVERSIONUTILS_HPP

#include "Engine/PassTypes.hpp"


#include <vulkan/vulkan.hpp>


vk::ImageLayout getVulkanImageLayout(const AttachmentType);

vk::Format getVulkanImageFormat(const Format);


Format getBellImageFormat(const vk::Format);

#endif
