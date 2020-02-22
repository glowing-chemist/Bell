#include "Engine/ForwardIBLTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


ForwardIBLTechnique::ForwardIBLTechnique(Engine* eng) :
	Technique("ForwardIBL", eng->getDevice()),
	mDesc{eng->getShader("./Shaders/ForwardMaterial.vert"),
		  eng->getShader("./Shaders/ForwardIBL.frag"),
		  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
		  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
			   true, BlendMode::None, BlendMode::None, false, DepthTest::LessEqual, Primitive::TriangleList },
	mTask("ForwardIBL", mDesc)
{
	mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
		VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

	mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mTask.addInput(kDFGLUT, AttachmentType::Texture2D);
	mTask.addInput(kSkyBox, AttachmentType::Texture2D);
	mTask.addInput(kConvolvedSkyBox, AttachmentType::Texture2D);
	mTask.addInput(kDefaultSampler, AttachmentType::Sampler);

	if (eng->isPassRegistered(PassType::Shadow))
		mTask.addInput(kShadowMap, AttachmentType::Texture2D);

	mTask.addInput(kMaterials, AttachmentType::ShaderResourceSet);
	mTask.addInput("model", AttachmentType::PushConstants);
    mTask.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    mTask.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);


	mTask.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Custom, LoadOp::Preserve);
}



void ForwardIBLTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>& meshes)
{
	mTask.clearCalls();

	for (const auto& mesh : meshes)
	{
		const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);

		mTask.addPushConsatntValue(mesh->mTransformation);
		mTask.addIndexedDrawCall(vertexOffset / mesh->mMesh->getVertexStride(), indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());
	}

	graph.addTask(mTask);
}
