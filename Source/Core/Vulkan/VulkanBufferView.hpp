#ifndef VK_BUFFER_VIEW_HPP
#define VK_BUFFER_VIEW_HPP

#include "Core/BufferView.hpp"
#include "Core/Buffer.hpp"
#include "MemoryManager.hpp"

#include <vulkan/vulkan.hpp>


class VulkanBufferView : public BufferViewBase
{
public:

	VulkanBufferView(Buffer&, const uint64_t offset = 0, const uint64_t size = VK_WHOLE_SIZE);
	~VulkanBufferView() = default;

	vk::Buffer getBuffer() const
	{
		return mBufferHandle;
	}

private:

	vk::Buffer mBufferHandle;
	Allocation mBufferMemory;
};

#endif