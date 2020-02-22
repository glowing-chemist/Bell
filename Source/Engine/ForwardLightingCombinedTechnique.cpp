#include "Engine/ForwardLightingCombinedTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


ForwardCombinedLightingTechnique::ForwardCombinedLightingTechnique(Engine* eng) :
	Technique("ForwardCombinedLighting", eng->getDevice()),
	mDesc{ eng->getShader("./Shaders/ForwardMaterialCombined.vert"),
		  eng->getShader("./Shaders/ForwardCombinedLighting.frag"),
		  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
		  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
			   getDevice()->getSwapChain()->getSwapChainImageHeight()},
			   true, BlendMode::None, BlendMode::None, false, DepthTest::LessEqual, Primitive::TriangleList },
	mTask("ForwardCombinedLighting", mDesc),
	mPointSampler(SamplerType::Point)
{
	mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
		VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

	mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mTask.addInput(kDFGLUT, AttachmentType::Texture2D);
	mTask.addInput(kActiveFroxels, AttachmentType::Texture2D);
	mTask.addInput(kSkyBox, AttachmentType::Texture2D);
	mTask.addInput(kConvolvedSkyBox, AttachmentType::Texture2D);
	mTask.addInput(kDefaultSampler, AttachmentType::Sampler);
	mTask.addInput("ForwardPointSampler", AttachmentType::Sampler);
	mTask.addInput(kSparseFroxels, AttachmentType::DataBufferRO);
	mTask.addInput(kLightIndicies, AttachmentType::DataBufferRO);

	if (eng->isPassRegistered(PassType::Shadow))
		mTask.addInput(kShadowMap, AttachmentType::Texture2D);

	mTask.addInput(kMaterials, AttachmentType::ShaderResourceSet);
	mTask.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);
	mTask.addInput("model", AttachmentType::PushConstants);
	mTask.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
	mTask.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);


	mTask.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, SizeClass::Custom, LoadOp::Preserve);
}



void ForwardCombinedLightingTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>& meshes)
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
