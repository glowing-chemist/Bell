#include "Engine/SSAOTechnique.hpp"
#include "Engine/UtilityTasks.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/Executor.hpp"

#include <cmath>

extern const char kSSAORaw[] = "SSAORaw";
extern const char kSSAOHistory[] = "SSAOHistory";
extern const char kSSAOCounter[] = "SSAOHistoryCounter";
extern const char kSSAOSampler[] = "SSAOSampler";
extern const char kSSAOBlurX[] = "SSAOBlurX";
extern const char kSSAOBlurY[] = "SSAOBlurY";

namespace
{
    float2 generateSphericalOffsets(const uint32_t n)
    {
        static float2 offsets[16]{
                float2(0.5381, 0.1856), float2(0.1379, 0.2486),
                float2(0.3371, 0.5679), float2(-0.6999, -0.0451),
                float2(0.0689, -0.1598), float2(0.0560, 0.0069),
                float2(-0.0146, 0.1402), float2(0.0100, -0.1924),
                float2(-0.3577, -0.5301), float2(-0.3169, 0.1063),
                float2(0.0103, -0.5869), float2(-0.0897, -0.4940),
                float2(0.7119, -0.0154), float2(-0.0533, 0.0596),
                float2(0.0352, -0.0631), float2(-0.4776, 0.2847) };

        return offsets[n];

        //return float2(((rand() / float(RAND_MAX)) - 0.5f) * 2.0f, ((rand() / float(RAND_MAX)) - 0.5f) * 2.0f);
    }
}


SSAOTechnique::SSAOTechnique(Engine* eng, RenderGraph& graph) :
	Technique{"SSAO", eng->getDevice()},
        mPipelineDesc{  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                        getDevice()->getSwapChain()->getSwapChainImageHeight() / 2},
                  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                  getDevice()->getSwapChain()->getSwapChainImageHeight() / 2}},
    mFulllscreenTriangleShader(eng->getShader("Shaders/FullScreenTriangle.vert")),
    mSSAOShader(eng->getShader("Shaders/SSAO.frag")),
    mFirstFrame(true),
    mSSAO{Image(getDevice(), Format::R8UNorm, ImageUsage::ColourAttachment | ImageUsage::Sampled, getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                getDevice()->getSwapChain()->getSwapChainImageHeight() / 2, 1, 1, 1, 1, "SSAO raw"),
          Image(getDevice(), Format::R8UNorm, ImageUsage::ColourAttachment | ImageUsage::Sampled | ImageUsage::TransferDest, getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                          getDevice()->getSwapChain()->getSwapChainImageHeight() / 2, 1, 1, 1, 1, "Prev SSAO raw"),
          Image(getDevice(), Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled, getDevice()->getSwapChain()->getSwapChainImageWidth(),
                          getDevice()->getSwapChain()->getSwapChainImageHeight(), 1, 1, 1, 1, "SSAO")},
    mSSAOViews{ImageView{mSSAO[0], ImageViewType::Colour}, ImageView{mSSAO[1], ImageViewType::Colour}, ImageView{mSSAO[2], ImageViewType::Colour}},
    mSSAOBlur{Image(getDevice(), Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled, getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                    getDevice()->getSwapChain()->getSwapChainImageHeight() / 2, 1, 1, 1, 1, "SSAO blur X"),
              Image(getDevice(), Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDest, getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                              getDevice()->getSwapChain()->getSwapChainImageHeight() / 2, 1, 1, 1, 1, "SSAO blur Y")},
    mSSAOBlurView{ImageView{mSSAOBlur[0], ImageViewType::Colour}, ImageView{mSSAOBlur[1], ImageViewType::Colour}},
    mHistoryCounter(getDevice(), Format::R32Uint, ImageUsage::Storage | ImageUsage::TransferDest, getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                    getDevice()->getSwapChain()->getSwapChainImageHeight() / 2, 1, 1, 1, 1, "SSAO Counter"),
    mHistoryCounterViews(mHistoryCounter, ImageViewType::Colour),
    mNearestSampler(SamplerType::Point),
    mSSAOBuffer(getDevice(), BufferUsage::Uniform, sizeof(SSAOBuffer), sizeof(SSAOBuffer), "SSAO Offsets"),
    mSSAOBufferView(mSSAOBuffer)
{
    mNearestSampler.setAddressModeU(AddressMode::Clamp);
    mNearestSampler.setAddressModeV(AddressMode::Clamp);
    mNearestSampler.setAddressModeW(AddressMode::Clamp);

    // full screen triangle so no vertex attributes.
    GraphicsTask task{"SSAO", mPipelineDesc};
    task.setVertexAttributes(0);

    task.addInput(kSSAOBuffer, AttachmentType::UniformBuffer);
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);

    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kPreviousLinearDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
    task.addInput(kSSAOHistory, AttachmentType::Texture2D);
    task.addInput(kSSAOCounter, AttachmentType::Image2D);
    task.addInput(kGBufferVelocity, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kSSAOSampler, AttachmentType::Sampler);

    task.addOutput(kSSAORaw, AttachmentType::RenderTarget2D, Format::R8UNorm, LoadOp::Nothing);

    task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mFulllscreenTriangleShader, nullptr, nullptr, nullptr, mSSAOShader);

            exec->draw(0, 3);
        }
    );
    graph.addTask(task);

    addBlurXTaskR8("SSAOBlurX", kSSAORaw, kSSAOBlurX, {getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                                                       getDevice()->getSwapChain()->getSwapChainImageHeight() / 2}, eng, graph);

    addBlurYTaskR8("SSAOBlurY", kSSAOBlurX, kSSAOBlurY, {getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
                                                       getDevice()->getSwapChain()->getSwapChainImageHeight() / 2}, eng, graph);

    addDeferredUpsampleTaskR8("Upsample SSAO", kSSAOBlurY, kSSAO, {getDevice()->getSwapChain()->getSwapChainImageWidth(),
                                                                 getDevice()->getSwapChain()->getSwapChainImageHeight()}, eng, graph);
}


void SSAOTechnique::render(RenderGraph&, Engine* eng)
{
    mHistoryCounter->updateLastAccessed();
    mHistoryCounterViews->updateLastAccessed();
    (*mSSAOBuffer)->updateLastAccessed();
    for(uint32_t i = 0; i < 3; ++i)
    {
        mSSAO[i]->updateLastAccessed();
        mSSAOViews[i]->updateLastAccessed();
    }
    for(uint32_t i = 0; i < 2; ++i)
    {
        mSSAOBlur[i]->updateLastAccessed();
        mSSAOBlurView[i]->updateLastAccessed();
    }

    if(mFirstFrame)
    {
        mHistoryCounter->clear(float4(0.0f, 0.0f, 0.0f, 0.0f));
        mSSAO[1]->clear(float4(1.0f, 1.0f, 1.0f, 1.0f));
        mFirstFrame = false;
    }

    const Camera& cam = eng->getCurrentSceneCamera();
    const ImageExtent extent = eng->getSwapChainImageView()->getImageExtent();
    const size_t index = eng->getDevice()->getCurrentSubmissionIndex() % 16;

    SSAOBuffer ssaoBuffer;

    const float frustumHeight = 2.0f * std::tan(glm::radians(cam.getFOV() * 0.5f));
    const float metrePixelHeight = (1.0f / frustumHeight) * float(extent.height);

    ssaoBuffer.projScale = metrePixelHeight;
    ssaoBuffer.radius = 3.0f;
    ssaoBuffer.bias = 0.01;
    ssaoBuffer.intensity = 1.0f;
    const float4x4 P = cam.getProjectionMatrix();
    const float4 projConstant
            (float(-2.0 / (extent.width * P[0][0])),
             float(-2.0 / (extent.height * P[1][1])),
             float((1.0 - (double)P[0][2]) / P[0][0]),
             float((1.0 + (double)P[1][2]) / P[1][1]));
    ssaoBuffer.projInfo = projConstant;
    ssaoBuffer.randomOffset = generateSphericalOffsets(index);

    (*mSSAOBuffer)->setContents(&ssaoBuffer, sizeof(SSAOBuffer));
}

