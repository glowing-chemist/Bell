#include "Engine/CommandContext.hpp"
#include "Engine/StaticMesh.h"
#include "Core/BellLogging.hpp"

#include <utility>


CommandContext::CommandContext() :
    mCurrentContextType{ContextType::Graphics},
    mCurrentRecordingState{RecordingState::Resources},
    mCurrentGraphicsPipelineState{},
    mCurrentComputePipelineState{},
    mCurrentResourceBindings{},
    mCurrentFrameBuffer{},
    mPushConstantsEnabled{false},
	mThunkedDrawCalls(),
	mThunkedDispatches(),
    mImages{},
    mImageArrays{},
    mBuffers{},
    mSamplers{}
{
	// preallocate some storage here
	mThunkedDrawCalls.reserve(50);
	mThunkedDispatches.reserve(10);

	reset();
}


CommandContext& CommandContext::setGraphicsPipelineState(const GraphicsPipelineDescription& state)
{
    BELL_ASSERT(mCurrentContextType == ContextType::Graphics, "Attempting to set pipeline state for incorrect context")

    if(mCurrentRecordingState == RecordingState::Commands)
            addTaskToGraph();

    mCurrentGraphicsPipelineState = state;

    return *this;
}


CommandContext& CommandContext::setCompuetPipelineState(const ComputePipelineDescription& state)
{
    BELL_ASSERT(mCurrentContextType == ContextType::Compute, "Attempting to set pipeline state for incorrect context \n")

    if(mCurrentRecordingState == RecordingState::Commands)
            addTaskToGraph();

    mCurrentComputePipelineState = state;

    return *this;
}


CommandContext& CommandContext::bindImageViews(const ImageView* view, const char* const* slots, uint32_t start, uint32_t count)
{
    BELL_ASSERT(start + count <= 16, "Attempting to bind image views out of bounds \n")

    if(mCurrentRecordingState == RecordingState::Commands)
            addTaskToGraph();

    mCurrentRecordingState = RecordingState::Resources;

    for(uint32_t i = 0; i < count; ++i)
    {
        BindingInfo binding{};
        binding.mName = slots[i];
        binding.mBound = true;

        // convert from extent to AttachmentType
        AttachmentType type = AttachmentType::Texture1D;
        ImageExtent extent = view[i]->getImageExtent();
        if(extent.height > 0)
            type = AttachmentType::Texture2D;
		if(extent.depth > 1)
            type = AttachmentType::Texture3D;

        binding.mType = type;

        mImages.insert({slots[i], view[i]});

        mCurrentResourceBindings[start + i] = binding;
    }

    return *this;
}


CommandContext& CommandContext::bindImageViewArrays(const ImageViewArray* view, const char* const* slots, uint32_t start, uint32_t count)
{
    BELL_ASSERT(start + count <= 16, "Attempting to bind image views out of bounds \n")

   if(mCurrentRecordingState == RecordingState::Commands)
       addTaskToGraph();

    mCurrentRecordingState = RecordingState::Resources;

    for(uint32_t i = 0; i < count; ++i)
    {
        BindingInfo binding{};
        binding.mName = slots[i];
		binding.mType = AttachmentType::TextureArray;
        binding.mBound = true;

        mImageArrays.insert({slots[i], view[i]});

        mCurrentResourceBindings[start + i] = binding;
    }

    return *this;
}


CommandContext& CommandContext::bindStorageTextureViews(const ImageView* view, const char* const* slots, uint32_t start, uint32_t count)
{
    BELL_ASSERT(start + count <= 16, "Attempting to bind storage texture views out of bounds \n")

    if(mCurrentRecordingState == RecordingState::Commands)
        addTaskToGraph();

    mCurrentRecordingState = RecordingState::Resources;

    for(uint32_t i = 0; i < count; ++i)
    {
        BindingInfo binding{};
        binding.mName = slots[i];

        // convert from extent to AttachmentType
        AttachmentType type = AttachmentType::Image1D;
        ImageExtent extent = view[i]->getImageExtent();
        if(extent.height > 0)
            type = AttachmentType::Image2D;
		if(extent.depth > 1)
            type = AttachmentType::Image3D;

        binding.mType = type;
        binding.mBound = true;

        mImages.insert({slots[i], view[i]});

        mCurrentResourceBindings[start + i] = binding;
    }

    return *this;
}


CommandContext& CommandContext::bindUniformBufferViews(const BufferView* view, const char* const* slots, uint32_t start, uint32_t count)
{
    BELL_ASSERT(start + count <= 16, "Attempting to bind uniform buffer views out of bounds \n")

    if(mCurrentRecordingState == RecordingState::Commands)
        addTaskToGraph();

    mCurrentRecordingState = RecordingState::Resources;

    for(uint32_t i = 0; i < count; ++i)
    {
        BindingInfo binding{};
        binding.mName = slots[i];
        binding.mType = AttachmentType::UniformBuffer;
        binding.mBound = true;

        mBuffers.insert({slots[i], view[i]});

        mCurrentResourceBindings[start + i] = binding;
    }

    return *this;
}


CommandContext& CommandContext::bindStorageBufferViews(const BufferView* view, const char* const* slots, uint32_t start, uint32_t count)
{
    BELL_ASSERT(start + count <= 16, "Attempting to bind storage buffers views out of bounds \n")

    if(mCurrentRecordingState == RecordingState::Commands)
        addTaskToGraph();

    mCurrentRecordingState = RecordingState::Resources;

    for(uint32_t i = 0; i < count; ++i)
    {
        BindingInfo binding{};
        binding.mName = slots[i];
        binding.mType = AttachmentType::DataBufferRW;
        binding.mBound = true;

        mBuffers.insert({slots[i], view[i]});

        mCurrentResourceBindings[start + i] = binding;
    }

    return *this;
}


CommandContext& CommandContext::bindSamplers(const Sampler* samp, const char* const* slots, uint32_t start, uint32_t count)
{
    BELL_ASSERT(start + count <= 16, "Attempting to bind samplers out of bounds \n")

    if(mCurrentRecordingState == RecordingState::Commands)
        addTaskToGraph();

    mCurrentRecordingState = RecordingState::Resources;

    for(uint32_t i = 0; i < count; ++i)
    {
        BindingInfo binding{};
        binding.mName = slots[i];
        binding.mType = AttachmentType::Sampler;
        binding.mBound = true;

        mSamplers.insert({slots[i], samp[i]});

        mCurrentResourceBindings[start + i] = binding;
    }

    return *this;
}


CommandContext& CommandContext::bindRenderTargets(const ImageView* view, const char* const* slots, const LoadOp* ops, uint32_t start, uint32_t count)
{
    BELL_ASSERT(start + count <= 16, "Attempting to bind image views out of bounds \n")

    if(mCurrentRecordingState == RecordingState::Commands)
        addTaskToGraph();

    mCurrentRecordingState = RecordingState::Resources;

    for(uint32_t i = 0; i < count; ++i)
    {
        RenderTargetInfo binding{};
        binding.mName = slots[i];
        binding.mBound = true;
        binding.mFormat = view[i]->getImageViewFormat();
		binding.mSize = SizeClass::Custom;
        binding.mLoadOp = ops[i];

		// convert from extent to AttachmentType
		AttachmentType type = AttachmentType::RenderTarget1D;
		ImageExtent extent = view[i]->getImageExtent();
		if(extent.height > 0)
			type = AttachmentType::RenderTarget2D;
		if(extent.depth > 1)
			type = AttachmentType::RenderTarget3D;

		binding.mType = type;

        mImages.insert({slots[i], view[i]});

        mCurrentFrameBuffer[start + i] = binding;
    }

    return *this;
}


CommandContext& CommandContext::bindImageViews(const char* const* slots, const uint32_t start, const uint32_t count)
{
	BELL_ASSERT(start + count <= 16, "Attempting to bind storage buffers views out of bounds \n")

	if(mCurrentRecordingState == RecordingState::Commands)
		addTaskToGraph();

	mCurrentRecordingState = RecordingState::Resources;

	for(uint32_t i = 0; i < count; ++i)
	{
		BindingInfo binding{};
		binding.mName = slots[i];
		binding.mType = AttachmentType::Texture2D;
		binding.mBound = true;

		mCurrentResourceBindings[start + i] = binding;
	}

	return *this;
}


CommandContext& CommandContext::bindImageViewArrays(const char* const* slots, const uint32_t start, const uint32_t count)
{
	BELL_ASSERT(start + count <= 16, "Attempting to bind storage buffers views out of bounds \n")

	if(mCurrentRecordingState == RecordingState::Commands)
		addTaskToGraph();

	mCurrentRecordingState = RecordingState::Resources;

	for(uint32_t i = 0; i < count; ++i)
	{
		BindingInfo binding{};
		binding.mName = slots[i];
		binding.mType = AttachmentType::TextureArray;
		binding.mBound = true;

		mCurrentResourceBindings[start + i] = binding;
	}

	return *this;
}


CommandContext& CommandContext::bindStorageTextureViews(const char* const * slots, const uint32_t start, const uint32_t count)
{
	BELL_ASSERT(start + count <= 16, "Attempting to bind storage buffers views out of bounds \n")

	if(mCurrentRecordingState == RecordingState::Commands)
		addTaskToGraph();

	mCurrentRecordingState = RecordingState::Resources;

	for(uint32_t i = 0; i < count; ++i)
	{
		BindingInfo binding{};
		binding.mName = slots[i];
		binding.mType = AttachmentType::Image2D;
		binding.mBound = true;

		mCurrentResourceBindings[start + i] = binding;
	}

	return *this;
}


CommandContext& CommandContext::bindRenderTargets(const OutputAttachmentDesc* descs, const uint32_t start, const uint32_t count)
{
	BELL_ASSERT(start + count <= 16, "Attempting to bind image views out of bounds \n")

	if(mCurrentRecordingState == RecordingState::Commands)
		addTaskToGraph();

	mCurrentRecordingState = RecordingState::Resources;

	for(uint32_t i = 0; i < count; ++i)
	{
		RenderTargetInfo binding{};
		binding.mName = descs[i].mSlot;
		binding.mBound = true;
		binding.mFormat = descs[i].mFormat;
		binding.mSize = descs[i].mSize;
		binding.mLoadOp = descs[i].mOp;

		binding.mType = AttachmentType::RenderTarget2D;

		mCurrentFrameBuffer[start + i] = binding;
	}

	return *this;
}


CommandContext& CommandContext::bindDepthStencilView(const OutputAttachmentDesc* descs, const uint32_t start, const uint32_t count)
{
	BELL_ASSERT(start + count <= 16, "Attempting to bind image views out of bounds \n")

	if(mCurrentRecordingState == RecordingState::Commands)
		addTaskToGraph();

	mCurrentRecordingState = RecordingState::Resources;

	for(uint32_t i = 0; i < count; ++i)
	{
		RenderTargetInfo binding{};
		binding.mName = descs[i].mSlot;
		binding.mBound = true;
		binding.mFormat = descs[i].mFormat;
		binding.mSize = descs[i].mSize;
		binding.mLoadOp = descs[i].mOp;

		binding.mType = AttachmentType::Depth;

		mCurrentFrameBuffer[start + i] = binding;
	}

	return *this;
}


CommandContext& CommandContext::bindDepthStencilView(const ImageView* view, const char* const* slots,  const LoadOp* ops, uint32_t start, uint32_t count)
{
	BELL_ASSERT(start + count <= 16, "Attempting to bind depth out of bounds \n")

	if(mCurrentRecordingState == RecordingState::Commands)
		addTaskToGraph();

	mCurrentRecordingState = RecordingState::Resources;

	for(uint32_t i = 0; i < count; ++i)
	{
		RenderTargetInfo binding{};
		binding.mName = slots[i];
		binding.mBound = true;
		binding.mFormat = view[i]->getImageViewFormat();
		binding.mSize = SizeClass::Custom;
		binding.mLoadOp = ops[i];
		binding.mType = AttachmentType::Depth;

		mImages.insert({slots[i], view[i]});

		mCurrentFrameBuffer[start + i] = binding;
	}

	return *this;
}


void CommandContext::draw(const uint32_t vertexOffset, const uint32_t numberOfVerticies)
{
    mThunkedDrawCalls.push_back({DrawType::Standard, vertexOffset, numberOfVerticies, 0, 0, 1, "", glm::mat4(1.0f)});

    mCurrentRecordingState = RecordingState::Commands;
}


void CommandContext::drawIndexed(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies)
{
    mThunkedDrawCalls.push_back({DrawType::Indexed, vertexOffset, 0, indexOffset, numberOfIndicies, 1, "", glm::mat4(1.0f)});

    mCurrentRecordingState = RecordingState::Commands;
}


void CommandContext::drawIndirect(const uint32_t drawCalls, const std::string&& indirectBuffer)
{
    mThunkedDrawCalls.push_back({DrawType::Indirect, 0, 0, 0, 0, drawCalls, indirectBuffer, glm::mat4(1.0f)});

    mCurrentRecordingState = RecordingState::Commands;
}


void CommandContext::drawIndexedInstanced(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies)
{
    mThunkedDrawCalls.push_back({DrawType::IndexedInstanced, vertexOffset, 0, indexOffset, numberOfIndicies, numberOfInstances, "", glm::mat4(1.0f)});

    mCurrentRecordingState = RecordingState::Commands;
}


void CommandContext::drawIndexedIndirect(const uint32_t drawCalls, const uint32_t indexOffset, const uint32_t numberOfIndicies, const std::string&& indirectName)
{
    mThunkedDrawCalls.push_back({DrawType::IndexedIndirect, 0, 0, indexOffset, numberOfIndicies, drawCalls, indirectName, glm::mat4(1.0f)});

    mCurrentRecordingState = RecordingState::Commands;
}


CommandContext& CommandContext::setPushConstantsEnable(const bool enabled)
{
    if(mCurrentRecordingState == RecordingState::Commands)
        addTaskToGraph();

    mPushConstantsEnabled = enabled;

    mCurrentRecordingState = RecordingState::Resources;

    return *this;
}


void CommandContext::setPushConstants(const glm::mat4& mat)
{
    mThunkedDrawCalls.push_back({DrawType::SetPushConstant, 0, 0, 0, 0, 0, "", mat});

    mCurrentRecordingState = RecordingState::Commands;
}


void CommandContext::dispatch(const uint32_t x, const uint32_t y, const uint32_t z)
{
    mThunkedDispatches.push_back({DispatchType::Standard, x, y, z, ""});

    mCurrentRecordingState = RecordingState::Commands;
}


void CommandContext::resetBindings()
{
    // Flush any pending work.
    if(!mThunkedDrawCalls.empty() || !mThunkedDispatches.empty())
	{
        addTaskToGraph();
	}

	for(auto& binding : mCurrentResourceBindings)
	{
		binding.mBound = false;
	}
	for(auto& target : mCurrentFrameBuffer)
	{
		target.mBound = false;
	}


    mPushConstantsEnabled = false;
    mCurrentVertexAttributes = 0;
}


void CommandContext::reset()
{
    mThunkedDrawCalls.clear();
    mThunkedDispatches.clear();

    mImages.clear();
    mImageArrays.clear();
    mBuffers.clear();
    mSamplers.clear();

	for(auto& binding : mCurrentResourceBindings)
	{
		binding.mBound = false;
	}
	for(auto& target : mCurrentFrameBuffer)
	{
		target.mBound = false;
	}

    mPushConstantsEnabled = false;
    mCurrentVertexAttributes = 0;
}


RenderGraph& CommandContext::finialise()
{
    if(!mThunkedDrawCalls.empty() || !mThunkedDispatches.empty())
	{
        addTaskToGraph();
	}

	for(const auto&[name, image] : mImages)
	{
		mRenderGraph.bindImage(name, image);
	}

	for(const auto&[name, imageArray] : mImageArrays)
	{
		mRenderGraph.bindImageArray(name, imageArray);
	}

	for(const auto&[name, buffer] :mBuffers)
	{
		mRenderGraph.bindBuffer(name, buffer);
	}

	for(const auto&[name, sampler] : mSamplers)
	{
		mRenderGraph.bindSampler(name, sampler);
	}

	mRenderGraph.compileDependancies();

    return mRenderGraph;
}


void CommandContext::addTaskToGraph()
{
    if(mCurrentContextType == ContextType::Graphics)
    {
        BELL_ASSERT(mCurrentGraphicsPipelineState, "No graphics pipeline bound")

        GraphicsTask task{mCurrentGraphicsPipelineState.value().mFragmentShader->getFilePath(),
                          mCurrentGraphicsPipelineState.value()};

        task.setVertexAttributes(mCurrentVertexAttributes);

        for(uint32_t i = 0; i < 16; ++i)
        {
            const BindingInfo& binding = mCurrentResourceBindings[i];

            // Bindings should be tightly packed.
            if(!binding.mBound)
                break;

            task.addInput(binding.mName, binding.mType);
        }

        for(uint32_t i = 0; i < 16; ++i)
        {
            const RenderTargetInfo& renderTarget = mCurrentFrameBuffer[i];

            if(!renderTarget.mBound)
                break;

			task.addOutput(renderTarget.mName, renderTarget.mType, renderTarget.mFormat, renderTarget.mSize, renderTarget.mLoadOp);
        }

		if(mPushConstantsEnabled)
		{
			task.addInput("PushConstants", AttachmentType::PushConstants);
		}

        for(const auto& call : mThunkedDrawCalls)
        {
            switch(call.mDrawType)
            {
                case DrawType::Standard:
                    task.addDrawCall(call.mVertexOffset, call.mNumberOfVerticies);
                    break;

                case DrawType::Instanced:
                    task.addInstancedDraw(call.mVertexOffset, call.mNumberOfVerticies, call.mNumberOfInstances);
                    break;

                case DrawType::Indexed:
                    task.addIndexedDrawCall(call.mVertexOffset, call.mIndexOffset, call.mNumberOfIndicies);
                    break;

                case DrawType::Indirect:
                    task.addIndirectDrawCall(call.mNumberOfInstances, std::move(call.mIndirectBufferName));
                    break;

                case DrawType::IndexedInstanced:
                    task.addIndexedInstancedDrawCall(call.mVertexOffset, call.mIndexOffset, call.mNumberOfInstances, call.mNumberOfIndicies);
                    break;

                case DrawType::IndexedIndirect:
                    task.addIndexedIndirectDrawCall(call.mNumberOfInstances, call.mIndexOffset, call.mNumberOfIndicies, std::move(call.mIndirectBufferName));
                    break;

                case DrawType::SetPushConstant:
                    task.addPushConsatntValue(&call.mPushConstantValue, sizeof(glm::mat4));
                    break;
            }
        }

        mRenderGraph.addTask(task);
    }
    else
    {
        BELL_ASSERT(mCurrentComputePipelineState, "No Compute pipeline bound")

        ComputeTask task{mCurrentComputePipelineState.value().mComputeShader->getFilePath(),
                          mCurrentComputePipelineState.value()};

        for(uint32_t i = 0; i < 16; ++i)
        {
            const BindingInfo& binding = mCurrentResourceBindings[i];

            // Bindings should be tightly packed.
            if(!binding.mBound)
                break;

            task.addInput(binding.mName, binding.mType);
        }

		if(mPushConstantsEnabled)
		{
			task.addInput("PushConstants", AttachmentType::PushConstants);
		}

        for(const auto& call : mThunkedDispatches)
        {
            switch(call.mDispatchType)
            {
                case DispatchType::Standard:
                    task.addDispatch(call.x, call.y, call.z);
                    break;

                case DispatchType::Indirect:
                    // Currently not handled for CommandContext.
                    BELL_TRAP;
                    break;
            }
        }

        mRenderGraph.addTask(task);
    }

    mThunkedDrawCalls.clear();
    mThunkedDispatches.clear();

	mCurrentRecordingState = RecordingState::Resources;
}
