#include "Engine/SSAOTechnique.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/Executor.hpp"

#define FIXED_SAMPLE_POINTS 1


namespace
{
    template<size_t N>
    std::array<float4, N> generateSphericalOffsets()
    {
#if FIXED_SAMPLE_POINTS
        std::array<float4, N> offsets{
                float4(0.5381, 0.1856, -0.4319, 0.0), float4(0.1379, 0.2486, 0.4430, 0.0),
                float4(0.3371, 0.5679, -0.0057, 0.0), float4(-0.6999, -0.0451, -0.0019, 0.0),
                float4(0.0689, -0.1598, -0.8547, 0.0), float4(0.0560, 0.0069, -0.1843, 0.0),
                float4(-0.0146, 0.1402, 0.0762, 0.0), float4(0.0100, -0.1924, -0.0344, 0.0),
                float4(-0.3577, -0.5301, -0.4358, 0.0), float4(-0.3169, 0.1063, 0.0158, 0.0),
                float4(0.0103, -0.5869, 0.0046, 0.0), float4(-0.0897, -0.4940, 0.3287, 0.0),
                float4(0.7119, -0.0154, -0.0918, 0.0), float4(-0.0533, 0.0596, -0.5411, 0.0),
                float4(0.0352, -0.0631, 0.5460, 0.0), float4(-0.4776, 0.2847, -0.0271, 0.0) };
#else
        std::array<float4, N> offsets;

        for (uint32_t i = 0; i < N; ++i)
        {
            const float r1 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) - 0.5f;
            const float r2 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) - 0.5f;
            const float r3 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) - 0.5f;

            const float3 vec = float3{ r1, r2, r3 };// glm::normalize(float3{ r1, r2, r3 });

            offsets[i] = float4{ vec, 0.0f };
        }
#endif

        return offsets;
    }
}


SSAOTechnique::SSAOTechnique(Engine* eng, RenderGraph& graph) :
	Technique{"SSAO", eng->getDevice()},
    	mPipelineDesc{eng->getShader("Shaders/FullScreenTriangle.vert")
                  ,eng->getShader("Shaders/SSAO.frag"),
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
						getDevice()->getSwapChain()->getSwapChainImageHeight() / 2},
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
				  getDevice()->getSwapChain()->getSwapChainImageHeight() / 2}},
    mBlurXDesc{eng->getShader("Shaders/blurXR8.comp")},
    mBlurYDesc{eng->getShader("Shaders/blurYR8.comp")},
    mSSAOBuffer(getDevice(), BufferUsage::Uniform, sizeof(SSAOBuffer), sizeof(SSAOBuffer), "SSAO Offsets"),
    mSSAOBufferView(mSSAOBuffer),
    mSSAO(getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::Storage, getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
          getDevice()->getSwapChain()->getSwapChainImageHeight() / 2, 1, 1, 1, 1, "SSAO"),
    mSSAOView(mSSAO, ImageViewType::Colour),
    mSSAOIntermediate(getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::Storage, getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
          getDevice()->getSwapChain()->getSwapChainImageHeight() / 2, 1, 1, 1, 1, "SSAOIntermediate"),
    mSSAOIntermediateView(mSSAOIntermediate, ImageViewType::Colour)
{
    // full screen triangle so no vertex attributes.
    GraphicsTask task{"SSAO", mPipelineDesc};
    task.setVertexAttributes(0);

    task.addInput(kSSAOBuffer, AttachmentType::UniformBuffer);
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);

    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);

    task.addOutput(kSSAORough, AttachmentType::RenderTarget2D, Format::R8UNorm, SizeClass::HalfSwapchain, LoadOp::Nothing);

    task.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            exec->draw(0, 3);
        }
    );
    graph.addTask(task);

    ComputeTask blurXTask{ "SSAOBlurX", mBlurXDesc };
    blurXTask.addInput(kSSAORough, AttachmentType::Texture2D);
    blurXTask.addInput(kSSAOBlurIntermidiate, AttachmentType::Image2D);
    blurXTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    blurXTask.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            exec->dispatch(std::ceil(eng->getDevice()->getSwapChain()->getSwapChainImageWidth() / 512.0f), eng->getDevice()->getSwapChain()->getSwapChainImageHeight() / 2, 1.0f);
        }
    );
    graph.addTask(blurXTask);

    ComputeTask blurYTask{ "SSAOBlurY", mBlurYDesc };
    blurYTask.addInput(kSSAOBlurIntermidiate, AttachmentType::Texture2D);
    blurYTask.addInput(kSSAO, AttachmentType::Image2D);
    blurYTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    blurYTask.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            exec->dispatch(eng->getDevice()->getSwapChain()->getSwapChainImageWidth() / 2, std::ceil(eng->getDevice()->getSwapChain()->getSwapChainImageHeight() / 512.0f), 1.0f);
        }
    );
    graph.addTask(blurYTask);
}


void SSAOTechnique::render(RenderGraph&, Engine*)
{
    (mSSAO)->updateLastAccessed();
    (mSSAOView)->updateLastAccessed();
    (mSSAOIntermediate)->updateLastAccessed();
    (mSSAOIntermediateView)->updateLastAccessed();

    SSAOBuffer ssaoBuffer;
    const auto offsets = generateSphericalOffsets<16>();
    ssaoBuffer.mScale = 0.002f;
    ssaoBuffer.mOffsetsCount = offsets.size();
    memcpy(ssaoBuffer.mOffsets, offsets.data(), sizeof(float4) * offsets.size());

    (*mSSAOBuffer)->setContents(&ssaoBuffer, sizeof(SSAOBuffer));
}


SSAOImprovedTechnique::SSAOImprovedTechnique(Engine* eng, RenderGraph& graph) :
    Technique{ "SSAO", eng->getDevice() },
    mPipelineDesc{ eng->getShader("Shaders/FullScreenTriangle.vert")
              ,eng->getShader("Shaders/SSAOImproved.frag"),
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                    getDevice()->getSwapChain()->getSwapChainImageHeight()},
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
              getDevice()->getSwapChain()->getSwapChainImageHeight()} },
    mBlurXDesc{eng->getShader("Shaders/blurXR8.comp")},
    mBlurYDesc{eng->getShader("Shaders/blurYR8.comp")},
    mSSAOBuffer(getDevice(), BufferUsage::Uniform, sizeof(SSAOBuffer), sizeof(SSAOBuffer), "SSAO Offsets"),
    mSSAOBufferView(mSSAOBuffer),
    mSSAO(getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::Storage, getDevice()->getSwapChain()->getSwapChainImageWidth(),
          getDevice()->getSwapChain()->getSwapChainImageHeight(), 1, 1, 1, 1, "SSAO"),
    mSSAOView(mSSAO, ImageViewType::Colour),
    mSSAOIntermediate(getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::Storage, getDevice()->getSwapChain()->getSwapChainImageWidth(),
          getDevice()->getSwapChain()->getSwapChainImageHeight(), 1, 1, 1, 1, "SSAOIntermediate"),
    mSSAOIntermediateView(mSSAOIntermediate, ImageViewType::Colour)
{
    GraphicsTask task{ "SSAO", mPipelineDesc };
    task.setVertexAttributes(0);

    task.addInput(kSSAOBuffer, AttachmentType::UniformBuffer);
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);

    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);

    task.addOutput(kSSAORough, AttachmentType::RenderTarget2D, Format::R8UNorm, SizeClass::Swapchain, LoadOp::Nothing);

    task.setRecordCommandsCallback(
        [](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            exec->draw(0, 3);
        }
    );
    graph.addTask(task);

    ComputeTask blurXTask{ "SSAOBlurX", mBlurXDesc };
    blurXTask.addInput(kSSAORough, AttachmentType::Texture2D);
    blurXTask.addInput(kSSAOBlurIntermidiate, AttachmentType::Image2D);
    blurXTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    blurXTask.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            exec->dispatch(std::ceil(eng->getDevice()->getSwapChain()->getSwapChainImageWidth() / 256.0f), eng->getDevice()->getSwapChain()->getSwapChainImageHeight(), 1.0f);
        }
    );
    graph.addTask(blurXTask);

    ComputeTask blurYTask{ "SSAOBlurY", mBlurYDesc };
    blurYTask.addInput(kSSAOBlurIntermidiate, AttachmentType::Texture2D);
    blurYTask.addInput(kSSAO, AttachmentType::Image2D);
    blurYTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    blurYTask.setRecordCommandsCallback(
        [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            exec->dispatch(eng->getDevice()->getSwapChain()->getSwapChainImageWidth(), std::ceil(eng->getDevice()->getSwapChain()->getSwapChainImageHeight() / 256.0f), 1.0f);
        }
    );
    graph.addTask(blurYTask);
}


void SSAOImprovedTechnique::render(RenderGraph&, Engine*)
{
    (mSSAO)->updateLastAccessed();
    (mSSAOView)->updateLastAccessed();
    (mSSAOIntermediate)->updateLastAccessed();
    (mSSAOIntermediateView)->updateLastAccessed();

    SSAOBuffer ssaoBuffer;
    const auto offsets = generateSphericalOffsets<16>();
    ssaoBuffer.mScale = 0.002f;
    ssaoBuffer.mOffsetsCount = offsets.size();
    memcpy(ssaoBuffer.mOffsets, offsets.data(), sizeof(float4) * offsets.size());

    (*mSSAOBuffer)->setContents(&ssaoBuffer, sizeof(SSAOBuffer));
}
