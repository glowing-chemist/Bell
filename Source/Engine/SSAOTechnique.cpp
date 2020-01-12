#include "Engine/SSAOTechnique.hpp"
#include "Core/RenderDevice.hpp"

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


SSAOTechnique::SSAOTechnique(Engine* eng) :
	Technique{"SSAO", eng->getDevice()},
    	mPipelineDesc{eng->getShader("Shaders/FullScreenTriangle.vert")
                  ,eng->getShader("Shaders/SSAO.frag"),
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
						getDevice()->getSwapChain()->getSwapChainImageHeight() / 2},
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
				  getDevice()->getSwapChain()->getSwapChainImageHeight() / 2}},
    mTask{"SSAO", mPipelineDesc},
    mSSAOBuffer(getDevice(), BufferUsage::Uniform, sizeof(SSAOBuffer), sizeof(SSAOBuffer), "SSAO Offsets"),
    mSSAOBufferView(mSSAOBuffer)
{
    // full screen triangle so no vertex attributes.
    mTask.setVertexAttributes(0);

    mTask.addInput(kSSAOBuffer, AttachmentType::UniformBuffer);
    mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);

    mTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
    mTask.addInput(kDefaultSampler, AttachmentType::Sampler);

	mTask.addOutput(kSSAO, AttachmentType::RenderTarget2D, Format::R8UNorm, SizeClass::HalfSwapchain, LoadOp::Clear_Black);

    mTask.addDrawCall(0, 3);
}


void SSAOTechnique::render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&)
{
    SSAOBuffer ssaoBuffer;
    const auto offsets = generateSphericalOffsets<16>();
    ssaoBuffer.mScale = 0.002f;
    ssaoBuffer.mOffsetsCount = offsets.size();
    memcpy(ssaoBuffer.mOffsets, offsets.data(), sizeof(float4) * offsets.size());

    MapInfo info{};
    info.mSize = sizeof(SSAOBuffer);

    void* SSAOBufferPtr = mSSAOBuffer.get()->map(info);

        std::memcpy(SSAOBufferPtr, &ssaoBuffer, sizeof(SSAOBuffer));

    mSSAOBuffer.get()->unmap();

    graph.addTask(mTask);
}


SSAOImprovedTechnique::SSAOImprovedTechnique(Engine* eng) :
    Technique{ "SSAO", eng->getDevice() },
    mPipelineDesc{ eng->getShader("Shaders/FullScreenTriangle.vert")
              ,eng->getShader("Shaders/SSAOImproved.frag"),
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                    getDevice()->getSwapChain()->getSwapChainImageHeight() / 2},
              Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
              getDevice()->getSwapChain()->getSwapChainImageHeight() / 2} },
    mTask{ "SSAO", mPipelineDesc },
    mSSAOBuffer(getDevice(), BufferUsage::Uniform, sizeof(SSAOBuffer), sizeof(SSAOBuffer), "SSAO Offsets"),
    mSSAOBufferView(mSSAOBuffer)
{
    mTask.setVertexAttributes(0);

    mTask.addInput(kSSAOBuffer, AttachmentType::UniformBuffer);
    mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);

    mTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
    mTask.addInput(kGBufferNormals, AttachmentType::Texture2D);
    mTask.addInput(kDefaultSampler, AttachmentType::Sampler);

    mTask.addOutput(kSSAO, AttachmentType::RenderTarget2D, Format::R8UNorm, SizeClass::HalfSwapchain, LoadOp::Clear_Black);

    mTask.addDrawCall(0, 3);
}


void SSAOImprovedTechnique::render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&)
{
    SSAOBuffer ssaoBuffer;
    const auto offsets = generateSphericalOffsets<16>();
    ssaoBuffer.mScale = 0.002f;
    ssaoBuffer.mOffsetsCount = offsets.size();
    memcpy(ssaoBuffer.mOffsets, offsets.data(), sizeof(float4) * offsets.size());

    MapInfo info{};
    info.mSize = sizeof(SSAOBuffer);

    void* SSAOBufferPtr = mSSAOBuffer.get()->map(info);

        std::memcpy(SSAOBufferPtr, &ssaoBuffer, sizeof(SSAOBuffer));

    mSSAOBuffer.get()->unmap();

    graph.addTask(mTask);
}
