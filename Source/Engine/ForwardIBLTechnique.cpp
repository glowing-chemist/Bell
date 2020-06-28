#include "Engine/ForwardIBLTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"


ForwardIBLTechnique::ForwardIBLTechnique(Engine* eng, RenderGraph& graph) :
	Technique("ForwardIBL", eng->getDevice()),
	mDesc{eng->getShader("./Shaders/ForwardMaterial.vert"),
		  eng->getShader("./Shaders/ForwardIBL.frag"),
		  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
		  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
			   true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList }
{
	GraphicsTask task{ "ForwardIBL", mDesc };
    task.setVertexAttributes(VertexAttributes::Position4 |
        VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	task.addInput(kDFGLUT, AttachmentType::Texture2D);
    task.addInput(kConvolvedSpecularSkyBox, AttachmentType::CubeMap);
    task.addInput(kConvolvedDiffuseSkyBox, AttachmentType::CubeMap);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);

    if (eng->isPassRegistered(PassType::Shadow) || eng->isPassRegistered(PassType::CascadingShadow))
		task.addInput(kShadowMap, AttachmentType::Texture2D);

	task.addInput(kMaterials, AttachmentType::ShaderResourceSet);
	task.addInput("model", AttachmentType::PushConstants);
	task.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
	task.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);


	task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	task.addOutput(kGBufferVelocity, AttachmentType::RenderTarget2D, Format::RG16UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Custom, LoadOp::Preserve);

	mTaskID = graph.addTask(task);
}



void ForwardIBLTechnique::render(RenderGraph& graph, Engine*)
{
	GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));

	task.setRecordCommandsCallback(
		[](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>& meshes)
		{
			exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
			exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

			for (const auto& mesh : meshes)
			{
                if (mesh->getMaterialFlags() & MaterialType::Transparent || !(mesh->getInstanceFlags() & InstanceFlags::Draw))
					continue;

				const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

				const MeshEntry entry = mesh->getMeshShaderEntry();

				exec->insertPushConsatnt(&entry, sizeof(MeshEntry));
				exec->indexedDraw(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
			}
		}
	);
}
