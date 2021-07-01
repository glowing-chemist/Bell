#include "Engine/ForwardIBLTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/UberShaderStateCache.hpp"
#include "Engine/DefaultResourceSlots.hpp"
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
	task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
	task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

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

                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const BufferView& pred = eng->getRenderGraph().getBuffer(kOcclusionPredicationBuffer);

                UberShaderCachedMaterialStateCache stateCache(exec, mMaterialPipelineVariants);

                for (uint32_t i = 0; i < meshes.size(); ++i)
                {
                    const auto& mesh = meshes[i];

                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());

                    exec->startCommandPredication(pred, i);

                    mesh->draw(exec, &stateCache, vertexOffset, indexOffset);

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

                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                UberShaderCachedMaterialStateCache stateCache(exec, mMaterialPipelineVariants);

                for (const auto& mesh : meshes)
                {
                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());
                    mesh->draw(exec, &stateCache, vertexOffset, indexOffset);
                }
            }
        );
    }

	mTaskID = graph.addTask(task);
}


void ForwardIBLTechnique::postGraphCompilation(RenderGraph& graph, RenderEngine* engine)
{
    const Scene* scene = engine->getScene();
    RenderDevice* device = engine->getDevice();
    const std::vector<Scene::Material>& materials = scene->getMaterialDescriptions();
    // compile pipelines for all material variants.
    const RenderTask& task = graph.getTask(mTaskID);
    Shader vertexShader = engine->getShader("./Shaders/ForwardMaterial.vert");
    for(const auto& material : materials)
    {
        if(mMaterialPipelineVariants.find(material.mMaterialTypes) == mMaterialPipelineVariants.end())
        {
            ShaderDefine materialDefine(L"MATERIAL_FLAGS", material.mMaterialTypes);
            Shader fragmentShader = engine->getShader("./Shaders/ForwardIBL.frag", materialDefine);

            const PipelineHandle pipeline = device->compileGraphicsPipeline(static_cast<const GraphicsTask&>(task),
                                                                            graph, vertexShader, nullptr,
                                                                            nullptr, nullptr, fragmentShader);

            mMaterialPipelineVariants.insert({material.mMaterialTypes, pipeline});
        }
    }
}
