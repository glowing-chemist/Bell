#include "Engine/ForwardLightingCombinedTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"


ForwardCombinedLightingTechnique::ForwardCombinedLightingTechnique(Engine* eng, RenderGraph& graph) :
	Technique("ForwardCombinedLighting", eng->getDevice()),
    mDesc{ eng->getShader("./Shaders/ForwardMaterial.vert"),
		  eng->getShader("./Shaders/ForwardCombinedLighting.frag"),
		  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
		  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
			   true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList },
	mPointSampler(SamplerType::Point)
{
	GraphicsTask task{ "ForwardCombinedLighting", mDesc };
	task.setVertexAttributes(VertexAttributes::Position4 |
		VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	task.addInput(kDFGLUT, AttachmentType::Texture2D);
	task.addInput(kLTCMat, AttachmentType::Texture2D);
	task.addInput(kLTCAmp, AttachmentType::Texture2D);
	task.addInput(kActiveFroxels, AttachmentType::Texture2D);
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::Texture2D);
    task.addInput(kConvolvedDiffuseSkyBox, AttachmentType::Texture2D);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);
	task.addInput("ForwardPointSampler", AttachmentType::Sampler);
	task.addInput(kSparseFroxels, AttachmentType::DataBufferRO);
	task.addInput(kLightIndicies, AttachmentType::DataBufferRO);

	if (eng->isPassRegistered(PassType::Shadow))
		task.addInput(kShadowMap, AttachmentType::Texture2D);

	task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
	task.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);
	task.addInput("model", AttachmentType::PushConstants);
	task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
	task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);


	task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	task.addOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Custom, LoadOp::Preserve);

	mTaskID = graph.addTask(task);
}



void ForwardCombinedLightingTechnique::render(RenderGraph& graph, Engine*)
{
	GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));

	task.setRecordCommandsCallback(
		[](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
		{
			exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
			exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

			for (const auto& mesh : meshes)
			{
                if (mesh->mMesh->getAttributes() & MeshAttributes::Transparent || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
					continue;

				const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

				const MeshEntry entry = mesh->getMeshShaderEntry();

				exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
				exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
			}
		}
	);
}
