#include "Engine/BlinnPhongTechnique.hpp"

#include "Core/RenderDevice.hpp"
#include "Engine/Engine.hpp"


BlinnPhongDeferredTexturesTechnique::BlinnPhongDeferredTexturesTechnique(Engine* eng) :
	Technique{ "BlinnPhongDeferredTextures", eng->getDevice() },

    mDepthName{kGBufferDepth},
    mVertexNormalsName{kGBufferNormals},
    mMaterialName{kGBufferMaterialID},
    mUVName{kGBufferUV},
	mPipelineDesc{ eng->getShader("./Shaders/FullScreentriangle.vert")
				  ,eng->getShader("./Shaders/BlinnPhongDeferredTexture.frag") ,
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
						getDevice()->getSwapChain()->getSwapChainImageHeight()},
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
				  getDevice()->getSwapChain()->getSwapChainImageHeight()} },
    mTask{ "BlinnPhongDeferredTextures", mPipelineDesc }
{
    // full scren triangle so no vertex attributes.
    mTask.setVertexAttributes(0);

    // These are always availble, and updated and bound by the engine.
	mTask.addInput(kLightBuffer, AttachmentType::UniformBuffer);
	mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
	mTask.addInput(kGBufferNormals, AttachmentType::Texture2D);
	mTask.addInput(kGBufferMaterialID, AttachmentType::Texture2D);
	mTask.addInput(kGBufferUV, AttachmentType::Texture2D);
	mTask.addInput(kSkyBox, AttachmentType::Texture2D);
	mTask.addInput(kConvolvedSkyBox, AttachmentType::Texture2D);
	mTask.addInput(kDefaultSampler, AttachmentType::Sampler);
	mTask.addInput(kMaterials, AttachmentType::ShaderResourceSet);

    // Bound by the engine.
    mTask.addInput(kDefaultSampler, AttachmentType::Sampler);

	mTask.addOutput(getLightTextureName(), AttachmentType::RenderTarget2D, Format::RGBA8SRGB, SizeClass::Swapchain, LoadOp::Clear_Black);

	mTask.addDrawCall(0, 3);
}


void BlinnPhongDeferredTexturesTechnique::render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance *>&)
{
	graph.addTask(mTask);
}
