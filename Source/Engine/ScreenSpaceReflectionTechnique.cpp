#include "Engine/ScreenSpaceReflectionTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"


constexpr const char SSRSampler[] = "SSRSampler";


ScreenSpaceReflectionTechnique::ScreenSpaceReflectionTechnique(Engine* eng, RenderGraph& graph) :
	Technique("SSR", eng->getDevice()),
    mClampedSampler(SamplerType::Linear)
{
    const auto viewPortX = eng->getSwapChainImage()->getExtent(0, 0).width / 2;
    const auto viewPortY = eng->getSwapChainImage()->getExtent(0, 0).height / 2;
	GraphicsPipelineDescription desc
	(
        Rect{ viewPortX, viewPortY },
		Rect{ viewPortX, viewPortY }
	);

    GraphicsTask task("SSR", desc);
    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferDiffuse, AttachmentType::Texture2D);
    task.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    task.addInput(SSRSampler, AttachmentType::Sampler);
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addOutput(kReflectionMap, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::HalfSwapchain, LoadOp::Nothing);

    task.setVertexAttributes(0);
    task.setRecordCommandsCallback(
      [](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
        {
            Shader vertexShader = eng->getShader("./Shaders/FullScreenTriangle.vert");
            Shader fragmentShader = eng->getShader("./Shaders/ScreenSpaceReflection.frag");
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, vertexShader, nullptr, nullptr, nullptr, fragmentShader);

            exec->draw(0, 3);
        }
    );

    graph.addTask(task);



    mClampedSampler.setAddressModeU(AddressMode::Clamp);
    mClampedSampler.setAddressModeV(AddressMode::Clamp);
    mClampedSampler.setAddressModeW(AddressMode::Clamp);
}


void ScreenSpaceReflectionTechnique::bindResources(RenderGraph& graph)
{
    if (!graph.isResourceSlotBound(SSRSampler))
    {
        graph.bindSampler(SSRSampler, mClampedSampler);
    }
}


RayTracedReflectionTechnique::RayTracedReflectionTechnique(Engine* eng, RenderGraph& graph) :
    Technique("Ray traced reflections", eng->getDevice()),
    mReflectionMap(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled,
                   eng->getSwapChainImage()->getExtent(0, 0).width / 4, eng->getSwapChainImage()->getExtent(0, 0).height / 4,
                   1, 1, 1, 1, "Reflection map"),
    mReflectionMapView(mReflectionMap, ImageViewType::Colour),
    mSampleNumber(0)
{
    ComputeTask task("Ray traced reflections");
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
    task.addInput(kBlueNoise, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kReflectionMap, AttachmentType::Image2D);
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    task.addInput("SampleNumber", AttachmentType::PushConstants);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput(kBVH, AttachmentType::ShaderResourceSet);

    task.setRecordCommandsCallback(
                [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    Shader computeShader = eng->getShader("./Shaders/RayTracedReflections.comp");
                    const RenderTask& task = graph.getTask(taskIndex);
                    exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, computeShader);

                    const uint32_t width =  eng->getDevice()->getSwapChain()->getSwapChainImageWidth() / 4;
                    const uint32_t height =  eng->getDevice()->getSwapChain()->getSwapChainImageHeight() / 4;

                    exec->insertPushConsatnt(&mSampleNumber, sizeof(uint32_t));
                    mSampleNumber = (mSampleNumber + 1) % 16;
                    exec->dispatch(std::ceil(width / 16.0f), std::ceil(height / 16.0f), 1);
                }
    );

    mTaskID = graph.addTask(task);
}


void RayTracedReflectionTechnique::bindResources(RenderGraph& graph)
{
    if (!graph.isResourceSlotBound(kReflectionMap))
    {
        graph.bindImage(kReflectionMap, mReflectionMapView);
    }
}
