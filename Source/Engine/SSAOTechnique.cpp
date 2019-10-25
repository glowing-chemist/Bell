#include "Engine/SSAOTechnique.hpp"
#include "Core/RenderDevice.hpp"


SSAOTechnique::SSAOTechnique(Engine* eng) :
	Technique{"SSAO", eng->getDevice()},
	mLinearSampler{SamplerType::Linear},
    	mPipelineDesc{eng->getShader("Shaders/FullScreenTriangle.vert")
                  ,eng->getShader("Shaders/SSAO.frag"),
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
						getDevice()->getSwapChain()->getSwapChainImageHeight() / 2},
				  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth() / 2,
				  getDevice()->getSwapChain()->getSwapChainImageHeight() / 2}},
    mTask{"SSAO", mPipelineDesc}
{
    // full scren triangle so no vertex attributes.
    mTask.setVertexAttributes(0);

    // These are always availble, and updated and bound by the engine.
    mTask.addInput("NormalsOffset", AttachmentType::UniformBuffer);
    mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);

    mTask.addInput(mDepthNameSlot, AttachmentType::Texture2D);
    mTask.addInput(kDefaultSampler, AttachmentType::Sampler);

	mTask.addOutput(kSSAO, AttachmentType::RenderTarget2D, Format::R8UNorm, SizeClass::HalfSwapchain, LoadOp::Clear_Black);

    mTask.addDrawCall(0, 3);
}


void SSAOTechnique::bindResources(RenderGraph& graph) const
{
    graph.bindSampler(kDefaultSampler, mLinearSampler);
}


void SSAOTechnique::render(RenderGraph& graph, Engine *, const std::vector<const Scene::MeshInstance *>& meshes)
{

}
