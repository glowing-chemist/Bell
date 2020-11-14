#include "Engine/LineariseDepthTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"

constexpr const char* kLinearDepthMip1 = "linearDepth1";
constexpr const char* kLinearDepthMip2 = "linearDepth2";
constexpr const char* kLinearDepthMip3 = "linearDepth3";
constexpr const char* kLinearDepthMip4 = "linearDepth4";
constexpr const char* kLinearDepthMip5 = "linearDepth5";
constexpr const char* kLinearDepthMip6 = "linearDepth6";
constexpr const char* kLinearDepthMip7 = "linearDepth7";
constexpr const char* kLinearDepthMip8 = "linearDepth8";
constexpr const char* kLinearDepthMip9 = "linearDepth9";
constexpr const char* kLinearDepthMip10 = "linearDepth10";
constexpr const char* kLinearDepthMip11 = "linearDepth11";

constexpr const char* mipNames[] = {kLinearDepthMip1, kLinearDepthMip2, kLinearDepthMip3, kLinearDepthMip4, kLinearDepthMip5, kLinearDepthMip6, kLinearDepthMip7,
                                   kLinearDepthMip8, kLinearDepthMip9, kLinearDepthMip10, kLinearDepthMip11};

constexpr const char* kPrevLinearDepthMip1 = "prevLinearDepth1";
constexpr const char* kPrevLinearDepthMip2 = "prevLinearDepth2";
constexpr const char* kPrevLinearDepthMip3 = "prevLinearDepth3";
constexpr const char* kPrevLinearDepthMip4 = "prevLinearDepth4";
constexpr const char* kPrevLinearDepthMip5 = "prevLinearDepth5";
constexpr const char* kPrevLinearDepthMip6 = "prevLinearDepth6";
constexpr const char* kPrevLinearDepthMip7 = "prevLinearDepth7";
constexpr const char* kPrevLinearDepthMip8 = "prevLinearDepth8";
constexpr const char* kPrevLinearDepthMip9 = "prevLinearDepth9";
constexpr const char* kPrevLinearDepthMip10 = "prevLinearDepth10";
constexpr const char* kPrevLinearDepthMip11 = "prevLinearDepth11";

constexpr const char* prevMipNames[] = {kPrevLinearDepthMip1, kPrevLinearDepthMip2, kPrevLinearDepthMip3, kPrevLinearDepthMip4, kPrevLinearDepthMip5, kPrevLinearDepthMip6, kPrevLinearDepthMip7,
                                   kPrevLinearDepthMip8, kPrevLinearDepthMip9, kPrevLinearDepthMip10, kPrevLinearDepthMip11};


constexpr const char* kOcclusionSampler = "OcclusionSampler";


LineariseDepthTechnique::LineariseDepthTechnique(Engine* eng, RenderGraph& graph) :
	Technique("linearise depth", eng->getDevice()),
    mMipLevels{static_cast<uint32_t>(std::ceil(std::log2(eng->getSwapChainImage()->getExtent(0, 0).height)))},
    mLineariseDepthShader(mMipLevels == 10 ? eng->getShader("./Shaders/LineariseDepth10.comp") : eng->getShader("./Shaders/LineariseDepth11.comp")),
    mLinearDepth(eng->getDevice(), Format::RG16Float, ImageUsage::Sampled | ImageUsage::Storage, eng->getSwapChainImage()->getExtent(0, 0).width, eng->getSwapChainImage()->getExtent(0, 0).height,
        1, mMipLevels, 1, 1, "Linear Depth"),
    mLinearDepthView(mLinearDepth, ImageViewType::Colour, 0, 1, 0, mMipLevels),
    mPreviousLinearDepth(eng->getDevice(), Format::RG16Float, ImageUsage::Sampled | ImageUsage::Storage | ImageUsage::TransferDest, eng->getSwapChainImage()->getExtent(0, 0).width, eng->getSwapChainImage()->getExtent(0, 0).height,
        1, mMipLevels, 1, 1, "Prev Linear Depth"),
    mPreviousLinearDepthView(mPreviousLinearDepth, ImageViewType::Colour, 0, 1, 0, mMipLevels),
    mFirstFrame(true),
    mOcclusionSampler(SamplerType::Point)
{    
    mOcclusionSampler.setAddressModeU(AddressMode::Clamp);
    mOcclusionSampler.setAddressModeV(AddressMode::Clamp);

    BELL_ASSERT(mMipLevels < 12, "Need to add more shader varientas")
    for(uint32_t i = 1; i < mMipLevels; ++i)
    {
        mMipsViews.push_back(ImageView(mLinearDepth, ImageViewType::Colour, 0, 1, i, 1));
        mPrevMipsViews.push_back(ImageView(mPreviousLinearDepth, ImageViewType::Colour, 0, 1, i, 1));
    }

    ComputeTask task{ "linearise depth" };
	task.addInput(kLinearDepth, AttachmentType::Image2D);
    for(uint32_t i = 1; i < mMipLevels; ++i)
    {
        task.addInput(mipNames[i - 1], AttachmentType::Image2D);
    }
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kOcclusionSampler, AttachmentType::Sampler);

	mTaskID = graph.addTask(task);
}


void LineariseDepthTechnique::render(RenderGraph& graph, Engine*)
{
    if(mFirstFrame)
    {
        mPreviousLinearDepth->clear(float4(INFINITY, INFINITY, INFINITY, 0.0f));
        mFirstFrame = false;
    }

	ComputeTask& task = static_cast<ComputeTask&>(graph.getTask(mTaskID));

    mLinearDepth->updateLastAccessed();
    mLinearDepthView->updateLastAccessed();
    for(auto& mip : mMipsViews)
        mip->updateLastAccessed();
    for(auto& mip : mPrevMipsViews)
        mip->updateLastAccessed();

	task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
		{
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mLineariseDepthShader);

			const auto extent = eng->getDevice()->getSwapChainImageView()->getImageExtent();
            exec->dispatch(std::ceil(extent.width / 16.0f), std::ceil(extent.height / 16.0f), 1);
		}
	);
}


void LineariseDepthTechnique::bindResources(RenderGraph& graph)
{
    const uint64_t submissionIndex = getDevice()->getCurrentSubmissionIndex();

    if((submissionIndex % 2) == 0)
    {
        graph.bindImage(kLinearDepth, mLinearDepthView);
        graph.bindImage(kPreviousLinearDepth, mPreviousLinearDepthView);

        for(uint32_t i = 0; i < mMipsViews.size(); ++i)
        {
            graph.bindImage(mipNames[i], mMipsViews[i], BindingFlags::ManualBarriers);
            graph.bindImage(prevMipNames[i], mPrevMipsViews[i], BindingFlags::ManualBarriers);
        }
    }
    else
    {
        graph.bindImage(kLinearDepth, mPreviousLinearDepthView);
        graph.bindImage(kPreviousLinearDepth, mLinearDepthView);

        for(uint32_t i = 0; i < mMipsViews.size(); ++i)
        {
            graph.bindImage(prevMipNames[i], mMipsViews[i], BindingFlags::ManualBarriers);
            graph.bindImage(mipNames[i], mPrevMipsViews[i], BindingFlags::ManualBarriers);
        }
    }

    if(!graph.isResourceSlotBound(kOcclusionSampler))
        graph.bindSampler(kOcclusionSampler, mOcclusionSampler);
}
