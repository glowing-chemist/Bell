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

class RenderGraph;
class RenderTask;
class Executor;
class ImageBase;
class BufferBase;


class RenderDevice
{
public:
	RenderDevice() :
		mCurrentSubmission{0},
		mFinishedSubmission{0},
		mCurrentFrameIndex{0},
		mCurrentPassIndex{0},
		mSwapChain{nullptr} {}

    virtual ~RenderDevice() = default;

	virtual void					   generateFrameResources(RenderGraph&, const uint64_t prefixHash) = 0;

	virtual void                       startPass(const RenderTask&) = 0;
	virtual Executor*				   getPassExecutor() = 0;
	virtual void					   freePassExecutor(Executor*) = 0;
	virtual void					   endPass() = 0;

	virtual void					   execute(BarrierRecorder& recorder) = 0;

	virtual void                       startFrame() = 0;
	virtual void                       endFrame() = 0;

	virtual void                       destroyImage(ImageBase& image) = 0;
	virtual void                       destroyImageView(ImageViewBase& view) = 0;

	virtual void                       destroyBuffer(BufferBase& buffer) = 0;

	virtual void					   destroyShaderResourceSet(const ShaderResourceSetBase& set) = 0;

    virtual void					   setDebugName(const std::string&, const uint64_t, const uint64_t objectType) = 0;

	virtual void                       flushWait() const = 0;

    virtual void					   submitFrame() = 0;
    virtual void					   swap() = 0;

	virtual size_t					   getMinStorageBufferAlignment() const = 0;

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

	uint64_t mCurrentPassIndex;

    SwapChainBase* mSwapChain;
};

#endif
