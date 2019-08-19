#ifndef RENDERDEVICE_HPP
#define RENDERDEVICE_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "BarrierManager.hpp"
#include "MemoryManager.hpp"
#include "DescriptorManager.hpp"
#include "SwapChain.hpp"
#include "CommandPool.h"
#include "Core/ImageView.hpp"
#include "Core/Sampler.hpp"
#include "Core/Pipeline.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/ComputeTask.hpp"
#include "RenderGraph/RenderGraph.hpp"

struct GLFWwindow;

struct QueueIndicies
{
	int GraphicsQueueIndex;
	int TransferQueueIndex;
	int ComputeQueueIndex;
};


struct GraphicsPipelineHandles
{
	std::shared_ptr<GraphicsPipeline> mGraphicsPipeline;
    vk::RenderPass mRenderPass;
    vk::DescriptorSetLayout mDescriptorSetLayout;
};

struct ComputePipelineHandles
{
	std::shared_ptr<ComputePipeline> mComputePipeline;
    vk::DescriptorSetLayout mDescriptorSetLayout;
};


class RenderDevice
{
public:
	RenderDevice(vk::Instance, vk::PhysicalDevice, vk::Device, vk::SurfaceKHR, GLFWwindow*);
    ~RenderDevice();

    vk::PhysicalDeviceLimits           getLimits() const { return mLimits; }

    void                               execute(RenderGraph&);

    void                               startFrame();
    void                               endFrame();

    vk::Image                          createImage(const vk::ImageCreateInfo& info)
                                            { return mDevice.createImage(info); }

    void                               destroyImage(Image& image) { mImagesPendingDestruction.push_back({image.getLastAccessed(), image.getImage(), image.getMemory(), nullptr}); }
    void                               destroyImageView(ImageView& view) { mImagesPendingDestruction.push_back({view.getLastAccessed(), view.getImage(), view.getImageMemory(), view.getImageView()}); }

    vk::ImageView                      createImageView(const vk::ImageViewCreateInfo& info)
                                            { return mDevice.createImageView(info); }

    void                               destroyImageView(vk::ImageView& view)
                                            { mDevice.destroyImageView(view); }

    Image&                             getSwapChainImage()
                                            { return getSwapChain()->getImage(mCurrentFrameIndex); }

    const Image&                       getSwapChainImage() const
                                            { return getSwapChain()->getImage(mCurrentFrameIndex); }

    ImageView&                         getSwapChainImageView()
                                            { return mSwapChain.getImageView(mSwapChain.getCurrentImageIndex()); }

    vk::Buffer                         createBuffer(const uint32_t, const vk::BufferUsageFlags);

    void                               destroyBuffer(Buffer& buffer) { mBuffersPendingDestruction.push_back({buffer.getLastAccessed(), buffer.getBuffer(), buffer.getMemory()}); }

    vk::CommandPool					   createCommandPool(const vk::CommandPoolCreateInfo& info)
											{ return mDevice.createCommandPool(info); }

	void							   destroyCommandPool(vk::CommandPool& pool)
											{ mDevice.destroyCommandPool(pool); }

    std::vector<vk::CommandBuffer>	   allocateCommandBuffers(const vk::CommandBufferAllocateInfo& info)
											{ return mDevice.allocateCommandBuffers(info); }

	void							   resetCommandPool(vk::CommandPool& pool)
											{ mDevice.resetCommandPool(pool, vk::CommandPoolResetFlags{}); }

    vk::ShaderModule                   createShaderModule(const vk::ShaderModuleCreateInfo& info)
                                            { return mDevice.createShaderModule(info); }

    void                               destroyShaderModule(vk::ShaderModule module)
                                            { mDevice.destroyShaderModule(module); }

    vk::DescriptorPool                 createDescriptorPool(const vk::DescriptorPoolCreateInfo& info)
                                            { return mDevice.createDescriptorPool(info); }

    void                               destroyDescriptorPool(vk::DescriptorPool& pool)
                                            { mDevice.destroyDescriptorPool(pool); }

    std::vector<vk::DescriptorSet>     allocateDescriptorSet(const vk::DescriptorSetAllocateInfo& info)
                                            { return mDevice.allocateDescriptorSets(info); }

    void                               writeDescriptorSets(std::vector<vk::WriteDescriptorSet>& writes)
                                            { mDevice.updateDescriptorSets(writes, {}); }

    vk::Sampler                        createSampler(const vk::SamplerCreateInfo& info)
                                            { return mDevice.createSampler(info); }

    vk::SwapchainKHR                   createSwapchain(const vk::SwapchainCreateInfoKHR& info)
                                            { return mDevice.createSwapchainKHR(info); }

    void                               destroySwapchain(vk::SwapchainKHR& swap)
                                            { mDevice.destroySwapchainKHR(swap); }

    std::vector<vk::Image>             getSwapchainImages(vk::SwapchainKHR& swap)
                                            { return mDevice.getSwapchainImagesKHR(swap); }

    uint32_t                           getSwapChainImageCount() const
                                            { return mSwapChain.getNumberOfSwapChainImages(); }

    void                               aquireNextSwapchainImage(vk::SwapchainKHR& swap,
                                                                uint64_t timout,
                                                                vk::Semaphore semaphore,
                                                                uint32_t& imageIndex)
                                            { mDevice.acquireNextImageKHR(swap, timout, semaphore, nullptr, &imageIndex); }

	vk::Pipeline						createPipeline(const vk::ComputePipelineCreateInfo& info)
	{
		return mDevice.createComputePipeline(nullptr, info);
	}

	vk::Pipeline						createPipeline(const vk::GraphicsPipelineCreateInfo& info)
	{
		return mDevice.createGraphicsPipeline(nullptr, info);
	}

    GraphicsPipelineHandles            createPipelineHandles(const GraphicsTask&);
    ComputePipelineHandles             createPipelineHandles(const ComputeTask&);

    // Accessors
    SwapChain*                         getSwapChain() { return &mSwapChain; }
    MemoryManager*                     getMemoryManager() { return &mMemoryManager; }
	CommandPool*					   getCurrentCommandPool() { return &mCommandPools[getCurrentFrameIndex()]; }
    vk::PhysicalDevice*                getPhysicalDevice() { return &mPhysicalDevice; }

    // Only these two can do usefull work when const
    const SwapChain*                         getSwapChain() const { return &mSwapChain; }
    const vk::PhysicalDevice*                getPhysicalDevice() const { return &mPhysicalDevice; }

    vk::Sampler                        getImmutableSampler(const Sampler& sampler);

	void							   setDebugName(const std::string&, const uint64_t, const vk::DebugReportObjectTypeEXT);


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

	void							   flushMemoryRange(const vk::MappedMemoryRange& range)
											{ mDevice.flushMappedMemoryRanges(1, &range); }

    void                               bindBufferMemory(vk::Buffer&, vk::DeviceMemory, const uint64_t);
    void                               bindImageMemory(vk::Image&, vk::DeviceMemory, const uint64_t);

    uint64_t						   getCurrentSubmissionIndex() const { return mCurrentSubmission; }
    uint64_t						   getCurrentFrameIndex() const { return mCurrentFrameIndex; }
    uint64_t                           getFinishedSubmissionIndex() const { return mFinishedSubmission; }

    void                               flushWait() const { mDevice.waitIdle(); }

	void							   execute(BarrierRecorder& recorder);

    void							   submitFrame();
    void							   swap();

	// non const as can compile uncompiled shaders.
	std::vector<vk::PipelineShaderStageCreateInfo>              generateShaderStagesInfo(GraphicsPipelineDescription&);
	std::vector<vk::PipelineShaderStageCreateInfo>              generateIndexedShaderStagesInfo(GraphicsPipelineDescription&);

private:

    vk::DescriptorSetLayout                                     generateDescriptorSetLayout(const RenderTask&);

	vk::PipelineLayout                                          generatePipelineLayout(vk::DescriptorSetLayout, const RenderTask&);

    vk::RenderPass                                              generateRenderPass(const GraphicsTask&);

    std::shared_ptr<GraphicsPipeline>                                            generatePipeline(const GraphicsTask&,
																				 const vk::DescriptorSetLayout descSetLayout,
																				 const vk::RenderPass&);

    std::shared_ptr<ComputePipeline>                                             generatePipeline(const ComputeTask&,
                                                                                 const vk::DescriptorSetLayout&);

	void														generateVulkanResources(RenderGraph&);

    void                                                        generateDescriptorSets(RenderGraph&);

    void                                                        generateFrameBuffers(RenderGraph&);

    std::vector<BarrierRecorder>                                recordBarriers(RenderGraph&);


    void                                                        clearDeferredResources();

    void														frameSyncSetup();

    vk::Fence                          createFence(const bool signaled);

    void                               destroyImage(vk::Image& image)
                                            { mDevice.destroyImage(image); }

    void                               destroyBuffer(vk::Buffer& buffer )
                                            { mDevice.destroyBuffer(buffer); }

    void                               destroyFrameBuffer(vk::Framebuffer& frameBuffer, uint64_t frameIndex)
                                            { mFramebuffersPendingDestruction.push_back({frameIndex, frameBuffer}); }

#ifndef NDEBUG
public:
    void                               insertDebugEventSignal(vk::CommandBuffer& buffer)
                                            { buffer.setEvent(mDebugEvent, vk::PipelineStageFlagBits::eAllCommands); }
    void                                waitOnDebugEvent()
    {
        while(mDevice.getEventStatus(mDebugEvent) != vk::Result::eEventSet) {}
        mDevice.resetEvent(mDebugEvent);
    }
private:
#endif

    // Keep track of when resources can be freed
    uint64_t mCurrentSubmission;
    uint64_t mFinishedSubmission;
    uint32_t mCurrentFrameIndex;

    std::deque<std::pair<uint64_t, vk::Framebuffer>> mFramebuffersPendingDestruction;

    struct ImageDestructionInfo
    {
        uint64_t mLastUsed;
        vk::Image mImageHandle;
        Allocation mImageMemory;
        vk::ImageView mCurrentImageView;
    };
    std::deque<ImageDestructionInfo> mImagesPendingDestruction;

    struct BufferDestructionInfo
    {
        uint64_t mLastUsed;
        vk::Buffer mBufferHandle;
        Allocation mBufferMemory;
    };
    std::deque<BufferDestructionInfo> mBuffersPendingDestruction;

	std::vector<vulkanResources> mVulkanResources;

    // underlying devices
    vk::Device mDevice;
    vk::PhysicalDevice mPhysicalDevice;

	vk::Instance mInstance;

    vk::Queue mGraphicsQueue;
    vk::Queue mComputeQueue;
    vk::Queue mTransferQueue;
	QueueIndicies mQueueFamilyIndicies;

    std::vector<vk::Fence>              mFrameFinished;
    std::vector<vk::Semaphore>          mImageAquired;
    std::vector<vk::Semaphore>          mImageRendered;

#ifndef NDEBUG
    vk::Event                           mDebugEvent;
#endif

    vk::PhysicalDeviceLimits mLimits;

    std::unordered_map<GraphicsPipelineDescription, GraphicsPipelineHandles> mGraphicsPipelineCache;
    std::unordered_map<ComputePipelineDescription, ComputePipelineHandles> mComputePipelineCache;

    std::unordered_map<Sampler, vk::Sampler> mImmutableSamplerCache;

	bool mHasDebugLableSupport;

    SwapChain mSwapChain;
    std::vector<CommandPool> mCommandPools;
    MemoryManager mMemoryManager;
    DescriptorManager mDescriptorManager;
};

#endif
