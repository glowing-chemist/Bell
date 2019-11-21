#include "Engine/GBufferMaterialTechnique.hpp"

#include "Engine/DefaultResourceSlots.hpp"


GBufferMaterialTechnique::GBufferMaterialTechnique(Engine* eng) :
    Technique{"GBufferMaterial", eng->getDevice()},
    mPipelineDescription{eng->getShader("./Shaders/GBufferPassThroughMaterial.vert"),
                         eng->getShader("./Shaders/GBufferMaterial.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::LessEqual, Primitive::TriangleList},

    mTask{"GBufferMaterial", mPipelineDescription}
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
                              VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    mTask.addInput("ModelMatrix", AttachmentType::PushConstants);

    mTask.addOutput(kGBufferNormals,  AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput(kGBufferUV,       AttachmentType::RenderTarget2D, Format::RGBA32Float, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput(kGBufferMaterialID, AttachmentType::RenderTarget2D, Format::R32Uint, SizeClass::Swapchain, LoadOp::Clear_Black);
    mTask.addOutput(kGBufferDepth,    AttachmentType::Depth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_White);
}


void GBufferMaterialTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance *>& meshes)
{
	mTask.clearCalls();

	uint64_t minVertexOffset = ~0ul;

	uint32_t vertexBufferOffset = 0;
	for (const auto& mesh : meshes)
	{
		const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);
		minVertexOffset = std::min(minVertexOffset, vertexOffset);

		mTask.addPushConsatntValue(mesh->mTransformation);
		mTask.addIndexedDrawCall(vertexBufferOffset, indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());

		vertexBufferOffset += mesh->mMesh->getVertexCount();
	}

	mTask.setVertexBufferOffset(minVertexOffset);

	graph.addTask(mTask);
}


GBufferMaterialPreDepthTechnique::GBufferMaterialPreDepthTechnique(Engine* eng) :
    Technique{"GBufferMaterialPreDepth", eng->getDevice()},

    mDepthName{kPreDepth},
    mPipelineDescription{eng->getShader("./Shaders/GBufferPassThroughMaterial.vert"),
                         eng->getShader("./Shaders/GBufferMaterial.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, false, DepthTest::LessEqual, Primitive::TriangleList},

    mTask{"GBufferMaterialPreDepth", mPipelineDescription}
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
                              VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    mTask.addInput("ModelMatrix", AttachmentType::PushConstants);

	mTask.addOutput(kGBufferNormals,  AttachmentType::RenderTarget2D, Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferUV,       AttachmentType::RenderTarget2D, Format::RGBA8SRGB, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(kGBufferMaterialID, AttachmentType::RenderTarget2D, Format::R32Uint, SizeClass::Swapchain, LoadOp::Clear_Black);
	mTask.addOutput(mDepthName,         AttachmentType::Depth, Format::D24S8Float, SizeClass::Swapchain, LoadOp::Preserve);

}


void GBufferMaterialPreDepthTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance *>& meshes)
{
	mTask.clearCalls();

	uint64_t minVertexOffset = ~0ul;

	uint32_t vertexBufferOffset = 0;
	for (const auto& mesh : meshes)
	{
		const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->mMesh);
		minVertexOffset = std::min(minVertexOffset, vertexOffset);

		mTask.addPushConsatntValue(mesh->mTransformation);
		mTask.addIndexedDrawCall(vertexBufferOffset, indexOffset / sizeof(uint32_t), mesh->mMesh->getIndexData().size());

		vertexBufferOffset += mesh->mMesh->getVertexCount();
	}

	mTask.setVertexBufferOffset(minVertexOffset);

	graph.addTask(mTask);
}
