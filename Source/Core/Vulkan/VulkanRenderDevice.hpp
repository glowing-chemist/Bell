#ifndef VK_RENDERDEVICE_HPP
#define VK_RENDERDEVICE_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Core/RenderDevice.hpp"
#include "Core/BarrierManager.hpp"
#include "MemoryManager.hpp"
#include "DescriptorManager.hpp"
#include "CommandPool.h"
#include "VulkanImageView.hpp"
#include "Core/Sampler.hpp"
#include "VulkanShaderResourceSet.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanImage.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanRenderInstance.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/ComputeTask.hpp"
#include "RenderGraph/RenderGraph.hpp"


// Debug option for finding which task is causing a device loss.
#define SUBMISSION_PER_TASK 0


struct GLFWwindow;


struct GraphicsPipelineHandles
{
	std::shared_ptr<GraphicsPipeline> mGraphicsPipeline;
    vk::RenderPass mRenderPass;
	std::vector< vk::DescriptorSetLayout> mDescriptorSetLayout;
};

struct ComputePipelineHandles
{
	std::shared_ptr<ComputePipeline> mComputePipeline;
	std::vector< vk::DescriptorSetLayout> mDescriptorSetLayout;
};


class VulkanRenderDevice : public RenderDevice
{
public:
	VulkanRenderDevice(vk::Instance, vk::PhysicalDevice, vk::Device, vk::SurfaceKHR, GLFWwindow*);
    ~VulkanRenderDevice();

	virtual void					   generateFrameResources(RenderGraph&) override;

	virtual void                       startPass(const RenderTask&) override;
	virtual Executor*				   getPassExecutor() override;
	virtual void					   freePassExecutor(Executor*) override;
	virtual void					   endPass() override;

    virtual void                       startFrame() override;
    virtual void                       endFrame() override;

    virtual void                       destroyImage(ImageBase& image) override 
	{	
		VulkanImage& vkImg = static_cast<VulkanImage&>(image);
		mImagesPendingDestruction.push_back({image.getLastAccessed(), vkImg.getImage(), vkImg.getMemory()});
	}
    virtual void                       destroyImageView(ImageViewBase& view) override
	{
		VulkanImageView& vkView = static_cast<VulkanImageView&>(view);
		mImageViewsPendingDestruction.push_back({view.getLastAccessed(), vkView.getImageView()});
	}

    vk::ImageView                      createImageView(const vk::ImageViewCreateInfo& info)
                                            { return mDevice.createImageView(info); }

    void                               destroyImageView(vk::ImageView& view)
                                            { mDevice.destroyImageView(view); }

    vk::Buffer                         createBuffer(const uint32_t, const vk::BufferUsageFlags);

    virtual void                       destroyBuffer(BufferBase& buffer) override
	{ 
		VulkanBuffer& vkBuffer = static_cast<VulkanBuffer&>(buffer);
		mBuffersPendingDestruction.push_back({buffer.getLastAccessed(), vkBuffer.getBuffer(), vkBuffer.getMemory()});
	}

	virtual void						destroyShaderResourceSet(const ShaderResourceSetBase& set) override
	{ 
		const VulkanShaderResourceSet& VkSRS = static_cast<const VulkanShaderResourceSet&>(set);
		mSRSPendingDestruction.push_back({ set.getLastAccessed(), VkSRS.getPool(), VkSRS.getLayout(), VkSRS.getDescriptorSet() });
	}

	virtual size_t					   getMinStorageBufferAlignment() const override
	{
		return getLimits().minStorageBufferOffsetAlignment;
	}

	vk::Image                          createImage(const vk::ImageCreateInfo& info)
	{
		return mDevice.createImage(info);
	}

    vk::CommandPool					   createCommandPool(const vk::CommandPoolCreateInfo& info)
											{ return mDevice.createCommandPool(info); }

	void							   destroyCommandPool(vk::CommandPool& pool)
											{ mDevice.destroyCommandPool(pool); }

    std::vector<vk::CommandBuffer>	   allocateCommandBuffers(const vk::CommandBufferAllocateInfo& info)
											{ return mDevice.allocateCommandBuffers(info); }

	void							   resetCommandPool(vk::CommandPool& pool)
											{ mDevice.resetCommandPool(pool, vk::CommandPoolResetFlags{}); }

	void							   resetDescriptorPool(vk::DescriptorPool& pool)
											{ mDevice.resetDescriptorPool(pool); }

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

	vk::Semaphore					   createSemaphore(const vk::SemaphoreCreateInfo& info)
											{ return mDevice.createSemaphore(info); }

	void							   destroySemaphore(const vk::Semaphore semaphore)
											{ mDevice.destroySemaphore(semaphore); }

    vk::Sampler                        createSampler(const vk::SamplerCreateInfo& info)
                                            { return mDevice.createSampler(info); }

    vk::SwapchainKHR                   createSwapchain(const vk::SwapchainCreateInfoKHR& info)
                                            { return mDevice.createSwapchainKHR(info); }

    void                               destroySwapchain(vk::SwapchainKHR& swap)
                                            { mDevice.destroySwapchainKHR(swap); }

    std::vector<vk::Image>             getSwapchainImages(vk::SwapchainKHR& swap)
                                            { return mDevice.getSwapchainImagesKHR(swap); }

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

    GraphicsPipelineHandles            createPipelineHandles(const GraphicsTask&, const RenderGraph&);
    ComputePipelineHandles             createPipelineHandles(const ComputeTask&, const RenderGraph&);

    // Accessors
    MemoryManager*                     getMemoryManager() { return &mMemoryManager; }
	CommandPool*					   getCurrentCommandPool() { return &mCommandPools[getCurrentFrameIndex()]; }
    vk::PhysicalDevice*                getPhysicalDevice() { return &mPhysicalDevice; }
	DescriptorManager*				   getDescriptorManager() { return &mDescriptorManager; }

    // Only these two can do usefull work when const
    const vk::PhysicalDevice*                getPhysicalDevice() const { return &mPhysicalDevice; }

    vk::Sampler                        getImmutableSampler(const Sampler& sampler);

    virtual void					   setDebugName(const std::string&, const uint64_t, const uint64_t) override;


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

	virtual void                       flushWait() const override { mGraphicsQueue.waitIdle();  mDevice.waitIdle(); }

	virtual void					   execute(BarrierRecorder& recorder) override;

    void							   submitFrame();
    void							   swap();

	// non const as can compile uncompiled shaders.
	std::vector<vk::PipelineShaderStageCreateInfo>              generateShaderStagesInfo(GraphicsPipelineDescription&);
	std::vector<vk::PipelineShaderStageCreateInfo>              generateIndexedShaderStagesInfo(GraphicsPipelineDescription&);

	vk::PhysicalDeviceLimits									getLimits() const { return mLimits; }

	template<typename B>
	vk::DescriptorSetLayout										generateDescriptorSetLayoutBindings(const std::vector<B>&);

private:

	vk::DescriptorSetLayout				                        generateDescriptorSetLayout(const RenderTask&);

	vk::PipelineLayout                                          generatePipelineLayout(const std::vector< vk::DescriptorSetLayout>&, const RenderTask&);

    vk::RenderPass                                              generateRenderPass(const GraphicsTask&);

    std::shared_ptr<GraphicsPipeline>                                            generatePipeline(const GraphicsTask&,
																				 const std::vector< vk::DescriptorSetLayout>& descSetLayout,
																				 const vk::RenderPass&);

    std::shared_ptr<ComputePipeline>                                             generatePipeline(const ComputeTask&,
                                                                                 const std::vector< vk::DescriptorSetLayout>&);

	void														generateVulkanResources(RenderGraph&);

    void                                                        generateDescriptorSets(RenderGraph&);

    virtual void                                                generateFrameBuffers(RenderGraph&);

	std::vector<vk::DescriptorSetLayout>						generateShaderResourceSetLayouts(const RenderTask&, const RenderGraph&);

    void                                                        clearDeferredResources();

	void														clearVulkanResources();

    void														frameSyncSetup();

    vk::Fence                          createFence(const bool signaled);

    void                               destroyImage(const vk::Image& image)
                                            { mDevice.destroyImage(image); }

    void                               destroyBuffer(const vk::Buffer& buffer )
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

    std::deque<std::pair<uint64_t, vk::Framebuffer>> mFramebuffersPendingDestruction;

    struct ImageDestructionInfo
    {
        uint64_t mLastUsed;
        vk::Image mImageHandle;
        Allocation mImageMemory;
    };
    std::deque<ImageDestructionInfo> mImagesPendingDestruction;

	struct ImageViewDestructionInfo
	{
		uint64_t mLastUsed;
		vk::ImageView mView;
	};
	std::deque<ImageViewDestructionInfo> mImageViewsPendingDestruction;

    struct BufferDestructionInfo
    {
        uint64_t mLastUsed;
        vk::Buffer mBufferHandle;
        Allocation mBufferMemory;
    };
    std::deque<BufferDestructionInfo> mBuffersPendingDestruction;

	struct ShaderResourceSetsInfo
	{
		uint64_t mLastUsed;
		vk::DescriptorPool mPool;
		vk::DescriptorSetLayout mLayout;
		vk::DescriptorSet mDescSet;
	};
	std::deque<ShaderResourceSetsInfo> mSRSPendingDestruction;

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

#ifndef NDEBUG
    vk::Event                           mDebugEvent;
#endif

    vk::PhysicalDeviceLimits mLimits;

    std::unordered_map<GraphicsPipelineDescription, GraphicsPipelineHandles> mGraphicsPipelineCache;
    std::unordered_map<ComputePipelineDescription, ComputePipelineHandles> mComputePipelineCache;

    std::unordered_map<Sampler, vk::Sampler> mImmutableSamplerCache;

	struct SwapChainInitializer
	{
		SwapChainInitializer(RenderDevice* device, vk::SurfaceKHR windowSurface, GLFWwindow* window, SwapChainBase** ptr)
		{
			*ptr = new VulkanSwapChain(device, windowSurface, window);
		}
	} mSwapChainInitializer;

    std::vector<CommandPool> mCommandPools;
    MemoryManager mMemoryManager;
    DescriptorManager mDescriptorManager;
};

#endif
