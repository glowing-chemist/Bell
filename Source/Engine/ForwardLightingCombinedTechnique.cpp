#include "Engine/ForwardLightingCombinedTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/UberShaderStateCache.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

constexpr const char kForwardPoitnSampler[] = "ForwardPointSampler";


ForwardCombinedLightingTechnique::ForwardCombinedLightingTechnique(RenderEngine* eng, RenderGraph& graph) :
	Technique("ForwardCombinedLighting", eng->getDevice()),
    mDesc{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
		  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
               FaceWindingOrder::CW, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList },
    mForwardCombinedVertexShader(eng->getShader("./Shaders/ForwardMaterial.vert")),
    mForwardCombinedFragmentShader(eng->getShader("./Shaders/ForwardCombinedLighting.frag")),
	mPointSampler(SamplerType::Point)
{
	GraphicsTask task{ "ForwardCombinedLighting", mDesc };
	task.setVertexAttributes(VertexAttributes::Position4 |
        VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo | VertexAttributes::Tangents);

	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	task.addInput(kDFGLUT, AttachmentType::Texture2D);
	task.addInput(kLTCMat, AttachmentType::Texture2D);
	task.addInput(kLTCAmp, AttachmentType::Texture2D);
	task.addInput(kActiveFroxels, AttachmentType::Texture2D);
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    task.addInput(kConvolvedDiffuseSkyBox, AttachmentType::CubeMap);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);
	task.addInput(kForwardPoitnSampler, AttachmentType::Sampler);
	task.addInput(kSparseFroxels, AttachmentType::DataBufferRO);
	task.addInput(kLightIndicies, AttachmentType::DataBufferRO);

    if (eng->isPassRegistered(PassType::Shadow) || eng->isPassRegistered(PassType::CascadingShadow) || eng->isPassRegistered(PassType::RayTracedShadows))
		task.addInput(kShadowMap, AttachmentType::Texture2D);

    if(eng->isPassRegistered(PassType::SSR) || eng->isPassRegistered(PassType::RayTracedReflections))
        task.addInput(kReflectionMap, AttachmentType::Texture2D);

	task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
	task.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);
	task.addInput("model", AttachmentType::PushConstants);
	task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
	task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);


    task.addManagedOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA16Float, SizeClass::Swapchain, LoadOp::Clear_Black);
	task.addManagedOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);

    if(eng->isPassRegistered(PassType::OcclusionCulling))
    {
        task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                PROFILER_EVENT("Forward combiuned lighting");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("Forward combiuned lighting");
            
                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                uint64_t currentMaterialFLags = ~0;

                const RenderTask& task = graph.getTask(taskIndex);
                Shader vertexShader = eng->getShader("./Shaders/ForwardMaterial.vert");

                const BufferView& pred = eng->getRenderGraph().getBuffer(kOcclusionPredicationBuffer);

                UberShaderMaterialStateCache stateCache(exec, eng, graph, task, vertexShader, "./Shaders/ForwardCombinedLighting.frag");

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
                PROFILER_EVENT("Forward combiuned lighting");
                PROFILER_GPU_TASK(exec);
                PROFILER_GPU_EVENT("Forward combiuned lighting");

                exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
                exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

                const RenderTask& task = graph.getTask(taskIndex);
                Shader vertexShader = eng->getShader("./Shaders/ForwardMaterial.vert");

                UberShaderMaterialStateCache stateCache(exec, eng, graph, task, vertexShader, "./Shaders/ForwardCombinedLighting.frag");

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


void ForwardCombinedLightingTechnique::bindResources(RenderGraph& graph)
{
    graph.bindSampler(kForwardPoitnSampler, mPointSampler);
}
