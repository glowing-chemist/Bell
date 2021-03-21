#ifndef RENDERDEVICE_HPP
#define RENDERDEVICE_HPP

#include <cstddef>
#include <cstdint>

#include "BarrierManager.hpp"
#include "SwapChain.hpp"
#include "Core/ImageView.hpp"
#include "Core/ShaderResourceSet.hpp"


// Debug option for finding which task is causing a device loss.
#define SUBMISSION_PER_TASK 0
// Enables timestamps around all tasks.
#define GPU_PROFILING 1

class RenderGraph;
class RenderTask;
class GraphicsTask;
class ComputeTask;
class Executor;
class ImageBase;
class BufferBase;
class Shader;
class CommandContextBase;

using PipelineHandle = uint64_t;

class RenderDevice
{
public:
	RenderDevice() :
		mCurrentSubmission{0},
		mFinishedSubmission{0},
		mCurrentFrameIndex{0},
		mSwapChain{nullptr} {}

    virtual ~RenderDevice() = default;

    virtual CommandContextBase*        getCommandContext(const uint32_t index, const QueueType) = 0;

    virtual PipelineHandle             compileGraphicsPipeline(const GraphicsTask& task,
                                                       const RenderGraph& graph,
                                                       const Shader& vertexShader,
                                                       const Shader* geometryShader,
                                                       const Shader* tessControl,
                                                       const Shader* tessEval,
                                                       const Shader& fragmentShader) = 0;

    virtual PipelineHandle             compileComputePipeline(const ComputeTask& task,
                                                       const RenderGraph& graph,
                                                       const Shader& compuetShader) = 0;

	virtual void                       startFrame() = 0;
	virtual void                       endFrame() = 0;

	virtual void                       destroyImage(ImageBase& image) = 0;
	virtual void                       destroyImageView(ImageViewBase& view) = 0;

	virtual void                       destroyBuffer(BufferBase& buffer) = 0;

	virtual void					   destroyShaderResourceSet(const ShaderResourceSetBase& set) = 0;

    virtual void					   setDebugName(const std::string&, const uint64_t, const uint64_t objectType) = 0;

	virtual void                       flushWait() const = 0;
    virtual void                       invalidatePipelines() = 0;

    virtual void					   submitContext(CommandContextBase*, const bool finalSubmission = false) = 0;
    virtual void					   swap() = 0;

	virtual size_t					   getMinStorageBufferAlignment() const = 0;
    virtual bool                       getHasCommandPredicationSupport() const = 0;
    virtual bool                       getHasAsyncComputeSupport() const = 0;

    virtual const std::vector<uint64_t>& getAvailableTimestamps() const = 0;
    virtual float                      getTimeStampPeriod() const = 0;

	Image& getSwapChainImage()
	{
		return getSwapChain()->getImage(mCurrentFrameIndex);
	}

	const Image& getSwapChainImage() const
	{
		return getSwapChain()->getImage(mCurrentFrameIndex);
	}

	ImageView& getSwapChainImageView()
	{
		return mSwapChain->getImageView(mSwapChain->getCurrentImageIndex());
	}

	const ImageView& getSwapChainImageView() const
	{
		return mSwapChain->getImageView(mSwapChain->getCurrentImageIndex());
	}

	uint64_t						   getCurrentSubmissionIndex() const { return mCurrentSubmission; }
	uint64_t						   getCurrentFrameIndex() const { return mCurrentFrameIndex; }
	uint64_t                           getFinishedSubmissionIndex() const { return mFinishedSubmission; }

	uint32_t                           getSwapChainImageCount() const
	{
		return mSwapChain->getNumberOfSwapChainImages();
	}

	// Accessors
	SwapChainBase* getSwapChain() { return mSwapChain; }
	const SwapChainBase* getSwapChain() const { return mSwapChain; }

protected:

    // Keep track of when resources can be freed
    uint64_t mCurrentSubmission;
    uint64_t mFinishedSubmission;
    uint32_t mCurrentFrameIndex;

    SwapChainBase* mSwapChain;
};

#endif
