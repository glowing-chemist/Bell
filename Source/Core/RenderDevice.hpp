#ifndef RENDERDEVICE_HPP
#define RENDERDEVICE_HPP

#include <cstddef>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "MemoryManager.hpp"
#include "SwapChain.hpp"
#include "CommandPool.h"
#include "Core/Image.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/ComputeTask.hpp"
#include "RenderGraph/RenderGraph.hpp"

class GLFWwindow;

struct QueueIndicies
{
	int GraphicsQueueIndex;
	int TransferQueueIndex;
	int ComputeQueueIndex;
};


struct GraphicsPipelineHandles
{
    vk::Pipeline mPipeline;
    vk::PipelineLayout mPipelineLayout;
    vk::RenderPass mRenderPass;
    vk::VertexInputBindingDescription mVertexBindingDescription;
    std::vector<vk::VertexInputAttributeDescription> mVertexAttributeDescription;
    vk::DescriptorSetLayout mDescriptorSetLayout;
};

struct ComputePipelineHandles
{
    vk::Pipeline mPipeline;
    vk::PipelineLayout mPipelineLayout;
    vk::DescriptorSetLayout mDescriptorSetLayout;
};


class RenderDevice
{
public:
    RenderDevice(vk::PhysicalDevice, vk::Device, vk::SurfaceKHR, GLFWwindow*);

    vk::Image                          createImage(const vk::Format,
                                                   const vk::ImageUsageFlags,
                                                   const vk::ImageType,
                                                   const uint32_t,
                                                   const uint32_t,
                                                   const uint32_t);

    void                               destroyImage(vk::Image& image)
                                            { mDevice.destroyImage(image); }

    vk::ImageView                      createImageView(const vk::ImageViewCreateInfo& info)
                                            { return mDevice.createImageView(info); }

    void                               destroyImageView(vk::ImageView& view)
                                            { mDevice.destroyImageView(view); }

    vk::Buffer                         createBuffer(const uint32_t, const vk::BufferUsageFlags);

    void                               destroyBuffer(vk::Buffer& buffer )
                                            { mDevice.destroyBuffer(buffer); }

	vk::CommandPool					   createCommandPool(vk::CommandPoolCreateInfo& info)
											{ return mDevice.createCommandPool(info); }

	void							   destroyCommandPool(vk::CommandPool& pool)
											{ mDevice.destroyCommandPool(pool); }

	std::vector<vk::CommandBuffer>	   allocateCommandBuffers(vk::CommandBufferAllocateInfo& info)
											{ return mDevice.allocateCommandBuffers(info); }

	void							   resetCommandPool(vk::CommandPool& pool)
											{ mDevice.resetCommandPool(pool, vk::CommandPoolResetFlags{}); }

    vk::ShaderModule                   createShaderModule(vk::ShaderModuleCreateInfo& info)
                                            { return mDevice.createShaderModule(info); }

    void                               destroyShaderModule(vk::ShaderModule module)
                                            { mDevice.destroyShaderModule(module); }

    vk::DescriptorPool                 createDescriptorPool(vk::DescriptorPoolCreateInfo& info)
                                            { return mDevice.createDescriptorPool(info); }

    void                               destroyDescriptorPool(vk::DescriptorPool& pool)
                                            { mDevice.destroyDescriptorPool(pool); }

    std::vector<vk::DescriptorSet>     allocateDescriptorSet(vk::DescriptorSetAllocateInfo& info)
                                            { return mDevice.allocateDescriptorSets(info); }

    GraphicsPipelineHandles            createPipelineHandles(GraphicsTask&);
    ComputePipelineHandles             createPipelineHandles(ComputeTask&);

    // Accessors
    SwapChain*                         getSwapChain() { return &mSwapChain; }
    MemoryManager*                     getMemoryManager() { return &mMemoryManager; }
	CommandPool*					   getCurrentCommandPool() { return &mCommandPools[mSwapChain.getCurrentImageIndex()]; }

    // Memory management functions
    vk::MemoryRequirements             getMemoryRequirements(vk::Image image)
                                            { return mDevice.getImageMemoryRequirements(image); }
    vk::MemoryRequirements             getMemoryRequirements(vk::Buffer buffer)
                                            { return mDevice.getBufferMemoryRequirements(buffer); }

    vk::Queue&                         getQueue(const QueueType);
	uint32_t						   getQueueFamilyIndex(const QueueType) const;

    vk::PhysicalDeviceMemoryProperties getMemoryProperties() const;
    vk::DeviceMemory                   allocateMemory(vk::MemoryAllocateInfo);
    void                               freeMemory(const vk::DeviceMemory);
    void*                              mapMemory(vk::DeviceMemory, vk::DeviceSize, vk::DeviceSize);
    void                               unmapMemory(vk::DeviceMemory);

    void                               bindBufferMemory(vk::Buffer&, vk::DeviceMemory, const uint64_t);
    void                               bindImageMemory(vk::Image&, vk::DeviceMemory, const uint64_t);

    uint64_t						   getCurrentFrameIndex() const { return mCurrentSubmission; }
private:

    std::pair<vk::VertexInputBindingDescription,
              std::vector<vk::VertexInputAttributeDescription>> generateVertexInput(const GraphicsTask&);
    vk::DescriptorSetLayout                                     generateDescriptorSetLayout(const RenderTask&);
    vk::PipelineLayout                                          generatePipelineLayout(vk::DescriptorSetLayout);
    vk::RenderPass                                              generateRenderPass(const GraphicsTask&);
    vk::PipelineRasterizationStateCreateInfo                    generateRasterizationInfo(const GraphicsTask&);
    std::vector<vk::PipelineShaderStageCreateInfo>              generateShaderStagesInfo(const GraphicsTask&);
    vk::Pipeline                                                generatePipeline(const GraphicsTask&,
                                                                                 const vk::PipelineLayout&,
                                                                                 const vk::VertexInputBindingDescription &,
                                                                                 const std::vector<vk::VertexInputAttributeDescription> &,
                                                                                 const vk::RenderPass&);
    vk::Pipeline                                                generatePipeline(const ComputeTask&,
                                                                                 const vk::PipelineLayout&);

    // Keep track of when resources can be freed
    uint64_t mCurrentSubmission;
    uint64_t mFinishedSubmission;

    // underlying devices
    vk::Device mDevice;
    vk::PhysicalDevice mPhysicalDevice;

    vk::Queue mGraphicsQueue;
    vk::Queue mComputeQueue;
    vk::Queue mTransferQueue;
	QueueIndicies mQueueFamilyIndicies;

	std::unordered_map<GraphicsPipelineDescription, std::pair<vk::Pipeline, vk::PipelineLayout>> mGraphicsPipelineCache;
	std::unordered_map<ComputePipelineDescription, std::pair<vk::Pipeline, vk::PipelineLayout>> mComputePipelineCache;

    SwapChain mSwapChain;
	std::vector<CommandPool> mCommandPools;
    MemoryManager mMemoryManager;
};

#endif
