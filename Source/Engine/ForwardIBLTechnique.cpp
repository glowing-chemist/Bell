#include "Engine/ForwardIBLTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


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
	task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
		VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	task.addInput(kDFGLUT, AttachmentType::Texture2D);
	task.addInput(kSkyBox, AttachmentType::Texture2D);
	task.addInput(kConvolvedSkyBox, AttachmentType::Texture2D);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);

	if (eng->isPassRegistered(PassType::Shadow))
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



void ForwardIBLTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>& meshes)
{
	GraphicsTask& task = static_cast<GraphicsTask&>(graph.getTask(mTaskID));
	task.clearCalls();

	for (const auto& mesh : meshes)
	{
		const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

        const MeshEntry entry = mesh->getMeshShaderEntry();

		task.addPushConsatntValue(&entry, sizeof(MeshEntry));
		task.addIndexedDrawCall(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
	}
}
