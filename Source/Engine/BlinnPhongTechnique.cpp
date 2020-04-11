#include "Engine/BlinnPhongTechnique.hpp"

#include "Core/RenderDevice.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"


BlinnPhongDeferredTexturesTechnique::BlinnPhongDeferredTexturesTechnique(Engine* eng, RenderGraph& graph) :
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
				  getDevice()->getSwapChain()->getSwapChainImageHeight()} }
{
	GraphicsTask task{ "BlinnPhongDeferredTextures", mPipelineDesc };
    // full scren triangle so no vertex attributes.
	task.setVertexAttributes(0);

    // These are always availble, and updated and bound by the engine.
	task.addInput(kLightBuffer, AttachmentType::UniformBuffer);
	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	task.addInput(kGBufferDepth, AttachmentType::Texture2D);
	task.addInput(kGBufferNormals, AttachmentType::Texture2D);
	task.addInput(kGBufferMaterialID, AttachmentType::Texture2D);
	task.addInput(kGBufferUV, AttachmentType::Texture2D);
	task.addInput(kSkyBox, AttachmentType::Texture2D);
	task.addInput(kConvolvedSkyBox, AttachmentType::Texture2D);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);
	task.addInput(kMaterials, AttachmentType::ShaderResourceSet);

    // Bound by the engine.
	task.addInput(kDefaultSampler, AttachmentType::Sampler);

	task.addOutput(getLightTextureName(), AttachmentType::RenderTarget2D, Format::RGBA8SRGB, SizeClass::Swapchain, LoadOp::Nothing);

	task.setRecordCommandsCallback(
		[](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
		{
			exec->draw(0, 3);
		}
	);
	mTaskID = graph.addTask(task);
}
