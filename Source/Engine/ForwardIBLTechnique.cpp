#include "Engine/ForwardIBLTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/UberShaderStateCache.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Engine/UtilityTasks.hpp"
#include "Core/Executor.hpp"


ForwardIBLTechnique::ForwardIBLTechnique(RenderEngine* eng, RenderGraph& graph) :
	Technique("ForwardIBL", eng->getDevice()),
    mDesc{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
		  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
               FaceWindingOrder::CW, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList }
{
	GraphicsTask task{ "ForwardIBL", mDesc };
    task.setVertexAttributes(VertexAttributes::Position4 |
        VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kBoneTransforms, AttachmentType::DataBufferRO);
    task.addInput(kInstanceTransformsBuffer, AttachmentType::DataBufferRO);
    task.addInput(kPreviousInstanceTransformsBuffer, AttachmentType::DataBufferRO);
	task.addInput(kDFGLUT, AttachmentType::Texture2D);
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    task.addInput(kConvolvedDiffuseSkyBox, AttachmentType::CubeMap);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);

    if (eng->isPassRegistered(PassType::Shadow) || eng->isPassRegistered(PassType::CascadingShadow) || eng->isPassRegistered(PassType::RayTracedShadows))
		task.addInput(kShadowMap, AttachmentType::Texture2D);

    if(eng->isPassRegistered(PassType::SSR) || eng->isPassRegistered(PassType::RayTracedReflections))
        task.addInput(kReflectionMap, AttachmentType::Texture2D);

	task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
	task.addInput("model", AttachmentType::PushConstants);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
        task.addInput(kOcclusionPredicationBuffer, AttachmentType::CommandPredicationBuffer);

    task.addManagedOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA16Float, SizeClass::Swapchain,
                          LoadOp::Clear_Black, StoreOp::Store,
                          ImageUsage::ColourAttachment | ImageUsage::Sampled | ImageUsage::TransferSrc);
	task.addManagedOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16Float, SizeClass::Swapchain, LoadOp::Clear_Black);
	task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("Forward IBL");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("Forward IBL");

                const BufferView& pred = eng->getRenderGraph().getBuffer(kOcclusionPredicationBuffer);

                UberShaderCachedPipelineStateCache stateCache(exec, mMaterialPipelineVariants);

                for (uint32_t i = 0; i < meshes.size(); ++i)
                {
                    const auto& mesh = meshes[i];

                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    exec->startCommandPredication(pred, i);

                    mesh->draw(exec, &stateCache);

                    exec->endCommandPredication();
                }
            }
        );
    }
    else
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("Forward IBL");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("Forward IBL");

                UberShaderCachedPipelineStateCache stateCache(exec, mMaterialPipelineVariants);

                for (const auto& mesh : meshes)
                {
                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    mesh->draw(exec, &stateCache);
                }
            }
        );
    }

	mTaskID = graph.addTask(task);
}


void ForwardIBLTechnique::postGraphCompilation(RenderGraph& graph, RenderEngine* engine)
{
    compileShadeFlagsPipelines(mMaterialPipelineVariants, "./Shaders/ForwardMaterial.vert", "./Shaders/ForwardIBL.frag", engine, graph, mTaskID);
}
