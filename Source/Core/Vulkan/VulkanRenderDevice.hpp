#ifndef VK_RENDERDEVICE_HPP
#define VK_RENDERDEVICE_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <unordered_map>
#include <vector>
#include <shared_mutex>
#include <vulkan/vulkan.hpp>

#include "Core/Profiling.hpp"
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
#include "VulkanAccelerationStructures.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/ComputeTask.hpp"
#include "RenderGraph/RenderGraph.hpp"


// Debug option for finding which task is causing a device loss.
#define SUBMISSION_PER_TASK 0


struct GLFWwindow;


class RenderDevice;
class Pipeline;


struct vulkanResources
{
#if ENABLE_LOGGING
    std::string mDebugName;
#endif
    std::shared_ptr<PipelineTemplate> mPipelineTemplate;
    std::vector<vk::DescriptorSetLayout> mDescSetLayout;
    // Only needed for graphics tasks
    std::optional<vk::RenderPass> mRenderPass;
    std::optional<vk::VertexInputBindingDescription> mVertexBindingDescription;
    std::optional<std::vector<vk::VertexInputAttributeDescription>> mVertexAttributeDescription;
};


struct GraphicsPipelineHandles
{
    std::shared_ptr<PipelineTemplate> mGraphicsPipelineTemplate;
    vk::RenderPass mRenderPass;
	std::vector< vk::DescriptorSetLayout> mDescriptorSetLayout;
};

struct ComputePipelineHandles
{
    std::shared_ptr<PipelineTemplate> mComputePipelineTemplate;
	std::vector< vk::DescriptorSetLayout> mDescriptorSetLayout;
};


class VulkanRenderDevice : public RenderDevice
{
public:
	VulkanRenderDevice(vk::Instance, vk::PhysicalDevice, vk::Device, vk::SurfaceKHR, vk::DynamicLoader&, GLFWwindow*, const uint32_t deviceFeatures);
    ~VulkanRenderDevice();

    virtual CommandContextBase*        getCommandContext(const uint32_t index, const QueueType) override;

    virtual PipelineHandle             compileGraphicsPipeline(const GraphicsTask& task,
                                                               const RenderGraph& graph,
                                                               const Shader& vertexShader,
                                                               const Shader* geometryShader,
                                                               const Shader* tessControl,
                                                               const Shader* tessEval,
                                                               const Shader& fragmentShader) override;

    virtual PipelineHandle             compileComputePipeline(const ComputeTask& task,
                                                              const RenderGraph& graph,
                                                              const Shader& computeShader) override;

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
        mImageViewsPendingDestruction.push_back({view.getLastAccessed(), vkView.getImageView(), vkView.getCubeMapImageView()});
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

	vk::AccelerationStructureKHR createAccelerationStructure(const vk::AccelerationStructureCreateInfoKHR& info) const
    {
        return mDevice.createAccelerationStructureKHR(info);
    }

    virtual void                       destroyBottomLevelAccelerationStructure(BottomLevelAccelerationStructureBase& structure) override
    {
        VulkanBottomLevelAccelerationStructure& VKAccel = static_cast<VulkanBottomLevelAccelerationStructure&>(structure);
        mAccelerationStructuresPendingDestruction.push_back({getCurrentSubmissionIndex(), VKAccel.getAccelerationStructureHandle()});
    }
    virtual void                       destroyTopLevelAccelerationStructure(TopLevelAccelerationStructureBase& structure) override
    {
        VulkanTopLevelAccelerationStructure& VKAccel = static_cast<VulkanTopLevelAccelerationStructure&>(structure);
        mAccelerationStructuresPendingDestruction.push_back({getCurrentSubmissionIndex(), VKAccel.getAccelerationStructureHandle()});
    }

    void buildAccelerationStructure(const uint32_t count,
                                    const vk::AccelerationStructureBuildGeometryInfoKHR* pInfos,
                                    const vk::AccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) const
    {
        mDevice.buildAccelerationStructuresKHR(vk::DeferredOperationKHR(), count,
                                               pInfos, ppBuildRangeInfos);
    }

    void                               destroyFrameBuffer(vk::Framebuffer& frameBuffer, uint64_t frameIndex)
                                            { mFramebuffersPendingDestruction.push_back({frameIndex, frameBuffer}); }

    void                              destroyPipeline(const vk::Pipeline pipeline)
                                            { mDevice.destroyPipeline(pipeline); }

	virtual size_t					   getMinStorageBufferAlignment() const override
	{
		return getLimits().minStorageBufferOffsetAlignment;
	}

    virtual bool                        getHasCommandPredicationSupport() const override
    {
        return mHasConditionalRenderingSupport;
    }

    virtual bool                       getHasAsyncComputeSupport() const override
    {
        return mGraphicsQueue != mComputeQueue;
    }

    virtual const std::vector<uint64_t>& getAvailableTimestamps() const override
    {
        return mFinishedTimeStamps;
    }

	vk::Image                          createImage(const vk::ImageCreateInfo& info)
	{
		return mDevice.createImage(info);
	}

    vk::Framebuffer                    createFrameBuffer(const RenderGraph &graph, const uint32_t taskIndex, const vk::RenderPass renderPass);


    vk::CommandPool					   createCommandPool(const vk::CommandPoolCreateInfo& info)
											{ return mDevice.createCommandPool(info); }

    vk::CommandBuffer                  getPrefixCommandBuffer(); // Returns the first command buffer that will be submitted this frame.

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
                                            { 
                                                vk::Result result =  mDevice.acquireNextImageKHR(swap, timout, semaphore, nullptr, &imageIndex);
                                                BELL_ASSERT(result == vk::Result::eSuccess, "Add error handling here TODO")
                                            }

	vk::Pipeline						createPipeline(const vk::ComputePipelineCreateInfo& info)
	{
        vk::ResultValue<vk::Pipeline> result = mDevice.createComputePipeline(nullptr, info);
        BELL_ASSERT(result.result == vk::Result::eSuccess, "Failed to create compute pipeline");

        return result.value;
	}

	vk::Pipeline						createPipeline(const vk::GraphicsPipelineCreateInfo& info)
	{
        vk::ResultValue<vk::Pipeline> result = mDevice.createGraphicsPipeline(nullptr, info);
        BELL_ASSERT(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline");

        return result.value;
	}

    GraphicsPipelineHandles            createPipelineHandles(const GraphicsTask&, const RenderGraph&);
    ComputePipelineHandles             createPipelineHandles(const ComputeTask&, const RenderGraph&);

    // Accessors
    MemoryManager*                     getMemoryManager() { return &mMemoryManager; }
    vk::PhysicalDevice*                getPhysicalDevice() { return &mPhysicalDevice; }
    DescriptorManager*				   getDescriptorManager() { return &mPermanentDescriptorManager; }

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

	virtual void                       flushWait() const override { mGraphicsQueue.waitIdle();  mDevice.waitIdle(); }
    virtual void                       invalidatePipelines() override;

    virtual void					   submitContext(CommandContextBase*, const bool finalSubmission = false) override;
    virtual void					   swap() override;

	vk::PhysicalDeviceLimits									getLimits() const { return mLimits; }

    vk::Semaphore                      getAsyncQueueSemaphore() const
    {
        return mAsyncQueueSemaphores[mCurrentFrameIndex];
    }

	template<typename B>
	vk::DescriptorSetLayout										generateDescriptorSetLayoutBindings(const std::vector<B>&, const TaskType type);

    vulkanResources                                             getTaskResources(const RenderGraph&, const RenderTask& task, const uint64_t prefixHash);
    vulkanResources                                             getTaskResources(const RenderGraph&, const uint32_t taskIndex, const uint64_t prefixHash);

    vk::QueryPool                                               createTimeStampPool(const uint32_t queries)
    {
        vk::QueryPoolCreateInfo info{};
        info.setFlags(vk::QueryPoolCreateFlags{});
        info.setQueryType(vk::QueryType::eTimestamp);
        info.setQueryCount(queries);
        info.setPipelineStatistics(vk::QueryPipelineStatisticFlags{});

        return mDevice.createQueryPool(info);
    }

    void                                                        destroyTimeStampPool(vk::QueryPool pool)
    {
        mDevice.destroyQueryPool(pool);
    }

    void                                                        writeTimeStampResults(const vk::QueryPool pool, uint64_t* data, const size_t queryCount)
    {
        if(queryCount == 0)
            return;

        mDevice.getQueryPoolResults(pool, 0, queryCount, sizeof(uint64_t) * queryCount, data, sizeof(uint64_t), vk::QueryResultFlagBits::e64);
    }

    float                                                       getTimeStampPeriod() const override
    {
        return mLimits.timestampPeriod;
    }

    vk::AccelerationStructureBuildSizesInfoKHR                  getAccelerationStructureMemoryRequirements(const vk::AccelerationStructureBuildTypeKHR type,
                                                                                                           const vk::AccelerationStructureBuildGeometryInfoKHR& geoemtryInfo,
                                                                                                           const std::vector<uint32_t>& maxPrimitives) const
    {
        return mDevice.getAccelerationStructureBuildSizesKHR(type, geoemtryInfo, maxPrimitives);
    }

    vk::Instance                                                getParentInstance() const
    {
        return mInstance;
    }

private:

	vk::DescriptorSetLayout				                        generateDescriptorSetLayout(const RenderTask&);

	vk::PipelineLayout                                          generatePipelineLayout(const std::vector< vk::DescriptorSetLayout>&, const RenderTask&);

    vk::RenderPass                                              generateRenderPass(const GraphicsTask&);

    vulkanResources generateVulkanResources(const RenderGraph &, const RenderTask& task);
    vulkanResources generateVulkanResources(const RenderGraph &, const uint32_t taskIndex);

	std::vector<vk::DescriptorSetLayout>						generateShaderResourceSetLayouts(const RenderTask&, const RenderGraph&);

    void                                                        clearDeferredResources();

    void														frameSyncSetup();

    vk::Fence                          createFence(const bool signaled);

    void                               destroyImage(const vk::Image& image)
                                            { mDevice.destroyImage(image); }

    void                               destroyBuffer(const vk::Buffer& buffer )
                                            { mDevice.destroyBuffer(buffer); }

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
        vk::ImageView mCubeMapView;
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

	struct AccelerationStructureDestructionInfo
    {
	    uint64_t mLastused;
	    vk::AccelerationStructureKHR mAccelerationStructure;
    };
	std::deque<AccelerationStructureDestructionInfo> mAccelerationStructuresPendingDestruction;

    std::shared_mutex mResourcesLock;
    std::unordered_map<uint64_t, vulkanResources> mVulkanResources;

    std::map<std::vector<vk::ImageView>, vk::Framebuffer> mFrameBufferCache;

    // underlying devices
    vk::Device mDevice;
    vk::PhysicalDevice mPhysicalDevice;
#if PROFILE // Just copies of the vkhpp handles for interfacing with optik
    VkDevice mProfileDeviceHandle;
    VkPhysicalDevice mProfilePhysicalDeviceHandle;
    VkQueue mProfileGraphicsQueue;

    Optick::VulkanFunctions mProfilingVulkanFunctions;
    void initProfilerVulkanFunctions(vk::DynamicLoader&);
#endif

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
    bool mHasConditionalRenderingSupport;

    std::unordered_map<Sampler, vk::Sampler> mImmutableSamplerCache;

	struct SwapChainInitializer
	{
		SwapChainInitializer(RenderDevice* device, vk::SurfaceKHR windowSurface, GLFWwindow* window, SwapChainBase** ptr)
		{
			*ptr = new VulkanSwapChain(device, windowSurface, window);
		}
	} mSwapChainInitializer;

    MemoryManager mMemoryManager;
    DescriptorManager mPermanentDescriptorManager;

    std::vector<std::vector<CommandContextBase*>> mGraphicsCommandContexts;
    std::vector<std::vector<CommandContextBase*>> mAsyncComputeCommandContexts;
    uint32_t mSubmissionCount;

    std::vector<vk::Semaphore> mAsyncQueueSemaphores;

    std::vector<uint64_t> mFinishedTimeStamps;
};

#endif
