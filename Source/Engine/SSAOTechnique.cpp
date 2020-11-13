#include "Engine/SSAOTechnique.hpp"
#include "Engine/UtilityTasks.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/Executor.hpp"

#include <cmath>

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
        mPipelineDesc{  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                        getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                  getDevice()->getSwapChain()->getSwapChainImageHeight()}},
    mFulllscreenTriangleShader(eng->getShader("Shaders/FullScreenTriangle.vert")),
    mSSAOShader(eng->getShader("Shaders/SSAO.frag")),
    mSSAOBuffer(getDevice(), BufferUsage::Uniform, sizeof(SSAOBuffer), sizeof(SSAOBuffer), "SSAO Offsets"),
    mSSAOBufferView(mSSAOBuffer)
{
    // full screen triangle so no vertex attributes.
    GraphicsTask task{"SSAO", mPipelineDesc};
    task.setVertexAttributes(0);

    task.addInput(kSSAOBuffer, AttachmentType::UniformBuffer);
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);

    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);

    task.addManagedOutput(kSSAO, AttachmentType::RenderTarget2D, Format::R8UNorm, SizeClass::Swapchain, LoadOp::Nothing);

    task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mFulllscreenTriangleShader, nullptr, nullptr, nullptr, mSSAOShader);

            exec->draw(0, 3);
        }
    );
    graph.addTask(task);
}


void SSAOTechnique::render(RenderGraph&, Engine* eng)
{
    const Camera& cam = eng->getCurrentSceneCamera();

    SSAOBuffer ssaoBuffer;

    const float frustumHeight = 2.0f * std::tan(glm::radians(cam.getFOV() * 0.5f));

    const ImageExtent extent = eng->getSwapChainImageView()->getImageExtent();
    ssaoBuffer.projScale = 500.0f;
    ssaoBuffer.radius = 3.0f;
    ssaoBuffer.bias = 0.01;
    ssaoBuffer.intensity = 0.6f;
    const float4x4 P = cam.getProjectionMatrix();
    const float4 projConstant
            (float(-2.0 / (extent.width * P[0][0])),
             float(-2.0 / (extent.height * P[1][1])),
             float((1.0 - (double)P[0][2]) / P[0][0]),
             float((1.0 + (double)P[1][2]) / P[1][1]));
    ssaoBuffer.projInfo = projConstant;

    (*mSSAOBuffer)->setContents(&ssaoBuffer, sizeof(SSAOBuffer));
}

