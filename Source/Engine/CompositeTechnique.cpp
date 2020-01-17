#include "Engine/CompositeTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "RenderGraph/GraphicsTask.hpp"


CompositeTechnique::CompositeTechnique(Engine* eng) :
	Technique("Composite", eng->getDevice()) {}


void CompositeTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>&)
{
	const auto viewPortX = eng->getSwapChainImage()->getExtent(0, 0).width;
	const auto viewPortY = eng->getSwapChainImage()->getExtent(0, 0).height;

	const bool usingGlobalLighting = eng->isPassRegistered(PassType::DeferredTextureAnalyticalPBRIBL) || eng->isPassRegistered(PassType::DeferredTexturePBRIBL) ||
		eng->isPassRegistered(PassType::DeferredTextureBlinnPhongLighting) || eng->isPassRegistered(PassType::ForwardIBL) || eng->isPassRegistered(PassType::DeferredPBRIBL);
	const bool usingAnalyticalLighting = eng->isPassRegistered(PassType::DeferredAnalyticalLighting);
	const bool usingSSAO = eng->isPassRegistered(PassType::SSAO) || eng->isPassRegistered(PassType::SSAOImproved);
	const bool usingOverlay = eng->isPassRegistered(PassType::Overlay);

    if(eng->debugTextureEnabled())
    {
        GraphicsPipelineDescription desc
        (
            eng->getShader("./Shaders/FullScreenTriangle.vert"),
            eng->getShader("./Shaders/CompositeOverlay.frag"),
            Rect{ viewPortX, viewPortY },
            Rect{ viewPortX, viewPortY }
        );

        GraphicsTask compositeTask("Composite", desc);
        compositeTask.addInput(eng->getDebugTextureSlot(), AttachmentType::Texture2D);
        compositeTask.addInput(kOverlay, AttachmentType::Texture2D);
        compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

        compositeTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Custom, LoadOp::Clear_Black);

        compositeTask.addDrawCall(0, 3);

        graph.addTask(compositeTask);
    }
	else if ((usingGlobalLighting != usingAnalyticalLighting) && usingSSAO && usingOverlay)
	{
		GraphicsPipelineDescription desc
		(
			eng->getShader("./Shaders/FullScreenTriangle.vert"),
			eng->getShader("./Shaders/CompositeOverlaySSAO.frag"),
			Rect{ viewPortX, viewPortY },
			Rect{ viewPortX, viewPortY }
		);

		GraphicsTask compositeTask("Composite", desc);
		compositeTask.addInput(usingGlobalLighting ? kGlobalLighting : kAnalyticLighting, AttachmentType::Texture2D);
		compositeTask.addInput(kSSAO, AttachmentType::Texture2D);
		compositeTask.addInput(kOverlay, AttachmentType::Texture2D);
		compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

		compositeTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Custom, LoadOp::Clear_Black);

		compositeTask.addDrawCall(0, 3);

		graph.addTask(compositeTask);
	}
    else if ((usingGlobalLighting != usingAnalyticalLighting) && usingOverlay)
	{
		GraphicsPipelineDescription desc
		(
			eng->getShader("./Shaders/FullScreenTriangle.vert"),
			eng->getShader("./Shaders/CompositeOverlay.frag"),
			Rect{ viewPortX, viewPortY },
			Rect{ viewPortX, viewPortY }
		);

		GraphicsTask compositeTask("Composite", desc);
		compositeTask.addInput(usingGlobalLighting ? kGlobalLighting : kAnalyticLighting, AttachmentType::Texture2D);
		compositeTask.addInput(kOverlay, AttachmentType::Texture2D);
		compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

		compositeTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Custom, LoadOp::Clear_Black);

		compositeTask.addDrawCall(0, 3);

		graph.addTask(compositeTask);
	}
	else if (usingOverlay)
	{
		GraphicsPipelineDescription desc
		(
			eng->getShader("./Shaders/FullScreenTriangle.vert"),
			eng->getShader("./Shaders/Blit.frag"),
			Rect{ viewPortX, viewPortY },
			Rect{ viewPortX, viewPortY }
		);

		GraphicsTask compositeTask("Composite", desc);
		compositeTask.addInput(kOverlay, AttachmentType::Texture2D);
		compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

		compositeTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Custom, LoadOp::Clear_Black);

		compositeTask.addDrawCall(0, 3);

		graph.addTask(compositeTask);
	}
	else if ((usingGlobalLighting != usingAnalyticalLighting))
	{
		GraphicsPipelineDescription desc
		(
			eng->getShader("./Shaders/FullScreenTriangle.vert"),
			eng->getShader("./Shaders/Blit.frag"),
			Rect{ viewPortX, viewPortY },
			Rect{ viewPortX, viewPortY }
		);

		GraphicsTask compositeTask("Composite", desc);
		compositeTask.addInput(usingGlobalLighting ? kGlobalLighting : kAnalyticLighting, AttachmentType::Texture2D);
		compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

		compositeTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Custom, LoadOp::Clear_Black);

		compositeTask.addDrawCall(0, 3);

		graph.addTask(compositeTask);
	}
}


void CompositeTechnique::bindResources(RenderGraph& graph) const
{
	graph.bindImage(kFrameBufer, getDevice()->getSwapChainImageView());
}
