#include "Engine/ScreenSpaceReflectionTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"


constexpr const char SSRSampler[] = "SSRSampler";


ScreenSpaceReflectionTechnique::ScreenSpaceReflectionTechnique(Engine* eng, RenderGraph& graph) :
	Technique("SSR", eng->getDevice()),
    mReflectionMap(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::ColourAttachment | ImageUsage::Sampled, eng->getSwapChainImage()->getExtent(0, 0).width, eng->getSwapChainImage()->getExtent(0, 0).height, 1, 1, 1, 1, "Reflection map"),
    mReflectionMapView(mReflectionMap, ImageViewType::Colour),
    mClampedSampler(SamplerType::Linear)
{
	const auto viewPortX = eng->getSwapChainImage()->getExtent(0, 0).width;
	const auto viewPortY = eng->getSwapChainImage()->getExtent(0, 0).height;
	GraphicsPipelineDescription desc
	(
		eng->getShader("./Shaders/FullScreenTriangle.vert"),
		eng->getShader("./Shaders/ScreenSpaceReflection.frag"),
		Rect{ viewPortX, viewPortY },
		Rect{ viewPortX, viewPortY }
	);

    GraphicsTask task("SSR", desc);
    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kGlobalLighting, AttachmentType::Texture2D);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
    task.addInput(SSRSampler, AttachmentType::Sampler);
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    if(eng->isPassRegistered(PassType::DeferredAnalyticalLighting))
        task.addInput(kAnalyticLighting, AttachmentType::Texture2D);
    task.addOutput(kReflectionMap, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Custom, LoadOp::Nothing);

    task.setVertexAttributes(0);
    task.setRecordCommandsCallback(
      [](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
        {
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
    if (!graph.isResourceSlotBound(kReflectionMap))
    {
        graph.bindImage(kReflectionMap, mReflectionMapView);
        graph.bindSampler(SSRSampler, mClampedSampler);
    }
}


RayTracedReflectionTechnique::RayTracedReflectionTechnique(Engine* eng, RenderGraph& graph) :
    Technique("Ray traced reflections", eng->getDevice()),
    mReflectionMap(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Storage | ImageUsage::Sampled,
                   eng->getSwapChainImage()->getExtent(0, 0).width / 4, eng->getSwapChainImage()->getExtent(0, 0).height / 4,
                   1, 1, 1, 1, "Reflection map"),
    mReflectionMapView(mReflectionMap, ImageViewType::Colour)
{
    ComputePipelineDescription pipelineDesc{eng->getShader("./Shaders/RayTracedReflections.comp")};

    ComputeTask task("Ray traced reflections", pipelineDesc);
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kReflectionMap, AttachmentType::Image2D);
    task.addInput(kSkyBox, AttachmentType::CubeMap);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput(kBVH, AttachmentType::ShaderResourceSet);

    task.setRecordCommandsCallback(
                [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    const uint32_t width =  eng->getDevice()->getSwapChain()->getSwapChainImageWidth() / 4;
                    const uint32_t height =  eng->getDevice()->getSwapChain()->getSwapChainImageHeight() / 4;

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
