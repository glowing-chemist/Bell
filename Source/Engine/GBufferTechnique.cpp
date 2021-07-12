#include "Engine/GBufferTechnique.hpp"
#include "Engine/UberShaderStateCache.hpp"
#include "Core/Executor.hpp"


GBufferTechnique::GBufferTechnique(RenderEngine* eng, RenderGraph& graph) :
	Technique{"GBuffer", eng->getDevice()},
    mMaterialPipelineVariants{},
        mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         FaceWindingOrder::CW, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList}
{
    GraphicsTask task{ "GBuffer", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kBoneTransforms, AttachmentType::DataBufferRO);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Model Matrix", AttachmentType::PushConstants);

    task.addManagedOutput(kGBufferDiffuse,    AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferNormals,     AttachmentType::RenderTarget2D, Format::RG8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferSpecularRoughness,   AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferVelocity,   AttachmentType::RenderTarget2D, Format::RG16Float, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferEmissiveOcclusion,   AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_ColourBlack_AlphaWhite);
    task.addManagedOutput(kGBufferDepth,      AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_Black, StoreOp::Store, ImageUsage::DepthStencil | ImageUsage::Sampled);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("gbuffer fill");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("gbuffer fill");

                const BufferView& pred = eng->getRenderGraph().getBuffer(kOcclusionPredicationBuffer);

                UberShaderCachedPipelineStateCache stateCache(exec, mMaterialPipelineVariants);

                for (uint32_t i = 0; i < meshes.size(); ++i)
                {
                    const auto* mesh = meshes[i];

                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    exec->startCommandPredication(pred, i);

                    mesh->draw(exec, &stateCache);

                    exec->endCommandPredication();
                }

                exec->setSubmitFlag();
            }
        );
    }
    else
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("gbuffer fill");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("gbuffer fill");

                UberShaderCachedPipelineStateCache stateCache(exec, mMaterialPipelineVariants);

                for (const auto& mesh : meshes)
                {
                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    mesh->draw(exec, &stateCache);

                   }

                exec->setSubmitFlag();
            }
        );
    }

    mTaskID = graph.addTask(task);
}


void GBufferTechnique::postGraphCompilation(RenderGraph& graph, RenderEngine* engine)
{
    const Scene* scene = engine->getScene();
    RenderDevice* device = engine->getDevice();
    const std::vector<Scene::Material>& materials = scene->getMaterialDescriptions();
    // compile pipelines for all material variants.
    const RenderTask& task = graph.getTask(mTaskID);
    Shader vertexShader = engine->getShader("./Shaders/GBufferPassThrough.vert");
    for(const auto& material : materials)
    {
        for(uint32_t skinning = 0; skinning < 2; ++skinning)
        {
            const uint64_t shadeflags = material.mMaterialTypes | (skinning ? kShade_Skinning : 0);
            if (mMaterialPipelineVariants.find(shadeflags) == mMaterialPipelineVariants.end()) {
                ShaderDefine materialDefine(L"SHADE_FLAGS", shadeflags);
                Shader fragmentShader = engine->getShader("./Shaders/GBuffer.frag", materialDefine);

                const PipelineHandle pipeline = device->compileGraphicsPipeline(static_cast<const GraphicsTask &>(task),
                                                                                graph, vertexShader, nullptr,
                                                                                nullptr, nullptr, fragmentShader);

                mMaterialPipelineVariants.insert({shadeflags, pipeline});
            }
        }
    }
}


GBufferPreDepthTechnique::GBufferPreDepthTechnique(RenderEngine* eng, RenderGraph& graph) :
    Technique{"GBufferPreDepth", eng->getDevice()},
    mMaterialPipelineVariants{},
        mPipelineDescription{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         FaceWindingOrder::CW, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList}
{
    GraphicsTask task{ "GBufferPreDepth", mPipelineDescription };

    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    task.addInput("Model Matrix", AttachmentType::PushConstants);

    task.addManagedOutput(kGBufferDiffuse, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferNormals, AttachmentType::RenderTarget2D, Format::RG8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferSpecularRoughness, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16Float, SizeClass::Swapchain, LoadOp::Clear_Black);
    task.addManagedOutput(kGBufferEmissiveOcclusion, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_ColourBlack_AlphaWhite);
    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, LoadOp::Preserve);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("gbuffer fill");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("gbuffer fill");

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

                exec->setSubmitFlag();
            }
        );
    }
    else
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("gbuffer fill");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("gbuffer fill");

                UberShaderCachedPipelineStateCache stateCache(exec, mMaterialPipelineVariants);

                for (const auto& mesh : meshes)
                {
                    if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                        continue;

                    mesh->draw(exec, &stateCache);
                  }

                exec->setSubmitFlag();
            }
        );
    }

    mTaskID = graph.addTask(task);
}


void GBufferPreDepthTechnique::postGraphCompilation(RenderGraph& graph, RenderEngine* engine)
{
    const Scene* scene = engine->getScene();
    RenderDevice* device = engine->getDevice();
    const std::vector<Scene::Material>& materials = scene->getMaterialDescriptions();
    // compile pipelines for all material variants.
    const RenderTask& task = graph.getTask(mTaskID);
    Shader vertexShader = engine->getShader("./Shaders/GBufferPassThrough.vert");
    for(const auto& material : materials)
    {
        if(mMaterialPipelineVariants.find(material.mMaterialTypes) == mMaterialPipelineVariants.end())
        {
            ShaderDefine materialDefine(L"SHADE_FLAGS", material.mMaterialTypes);
            Shader fragmentShader = engine->getShader("./Shaders/GBuffer.frag", materialDefine);

            const PipelineHandle pipeline = device->compileGraphicsPipeline(static_cast<const GraphicsTask&>(task),
                                                                            graph, vertexShader, nullptr,
                                                                            nullptr, nullptr, fragmentShader);

            mMaterialPipelineVariants.insert({material.mMaterialTypes, pipeline});
        }
    }
}

