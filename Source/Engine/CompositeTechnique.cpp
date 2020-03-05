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
		eng->isPassRegistered(PassType::DeferredTextureBlinnPhongLighting) || eng->isPassRegistered(PassType::ForwardIBL) || eng->isPassRegistered(PassType::DeferredPBRIBL)
		|| eng->isPassRegistered(PassType::ForwardCombinedLighting);
	const bool usingAnalyticalLighting = eng->isPassRegistered(PassType::DeferredAnalyticalLighting);
	const bool usingSSAO = eng->isPassRegistered(PassType::SSAO) || eng->isPassRegistered(PassType::SSAOImproved);
	const bool usingOverlay = eng->isPassRegistered(PassType::Overlay);
	const bool usingTAA = eng->isPassRegistered(PassType::TAA);

    if (eng->debugTextureEnabled())
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
	else if ((usingGlobalLighting != usingAnalyticalLighting) && usingSSAO && usingOverlay && !usingTAA)
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
    else if ((usingGlobalLighting != usingAnalyticalLighting) && usingOverlay && !usingTAA)
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
	else if (usingGlobalLighting && usingAnalyticalLighting && usingOverlay && usingSSAO && !usingTAA)
	{
		GraphicsPipelineDescription desc
		(
			eng->getShader("./Shaders/FullScreenTriangle.vert"),
			eng->getShader("./Shaders/CompositeOverlayCombinedLightingSSAO.frag"),
			Rect{ viewPortX, viewPortY },
			Rect{ viewPortX, viewPortY }
		);

		GraphicsTask compositeTask("Composite", desc);
		compositeTask.addInput(kGlobalLighting, AttachmentType::Texture2D);
		compositeTask.addInput(kAnalyticLighting, AttachmentType::Texture2D);
		compositeTask.addInput(kSSAO, AttachmentType::Texture2D);
		compositeTask.addInput(kOverlay, AttachmentType::Texture2D);
		compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

		compositeTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Custom, LoadOp::Clear_Black);

		compositeTask.addDrawCall(0, 3);

		graph.addTask(compositeTask);
	}
	else if ((usingGlobalLighting != usingAnalyticalLighting) && !usingTAA && !usingOverlay)
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
	else if (usingGlobalLighting && usingAnalyticalLighting && usingOverlay && usingSSAO && usingTAA)
	{
		// Just combine the lighting passes so that we can use it as an input for the TAA.
		GraphicsPipelineDescription desc
		(
			eng->getShader("./Shaders/FullScreenTriangle.vert"),
			eng->getShader("./Shaders/LightingCompositeSSAO.frag"),
			Rect{ viewPortX, viewPortY },
			Rect{ viewPortX, viewPortY }
		);

		GraphicsTask compositeTask("LightingComposite", desc);
		compositeTask.addInput(kGlobalLighting, AttachmentType::Texture2D);
		compositeTask.addInput(kAnalyticLighting, AttachmentType::Texture2D);
		compositeTask.addInput(kSSAO, AttachmentType::Texture2D);
		compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);
		compositeTask.addOutput(kCompositeOutput, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Swapchain, LoadOp::Clear_Black);
		compositeTask.addDrawCall(0, 3);

		graph.addTask(compositeTask);

		// Now add the overlay the final TAA'd result to the framebuffer.
		GraphicsPipelineDescription overlayDesc
		(
			eng->getShader("./Shaders/FullScreenTriangle.vert"),
			eng->getShader("./Shaders/CompositeOverlay.frag"),
			Rect{ viewPortX, viewPortY },
			Rect{ viewPortX, viewPortY }
		);

		GraphicsTask overlayTask("OverlayComposite", overlayDesc);
		overlayTask.addInput(kNewTAAHistory, AttachmentType::Texture2D);
		overlayTask.addInput(kOverlay, AttachmentType::Texture2D);
		overlayTask.addInput(kDefaultSampler, AttachmentType::Sampler);
		overlayTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Custom, LoadOp::Clear_Black);
		overlayTask.addDrawCall(0, 3);

		graph.addTask(overlayTask);
	}
	else if ((usingGlobalLighting != usingAnalyticalLighting) && usingOverlay && usingSSAO && usingTAA)
	{
		// Just combine the lighting passes so that we can use it as an input for the TAA.
		GraphicsPipelineDescription desc
		(
			eng->getShader("./Shaders/FullScreenTriangle.vert"),
			eng->getShader("./Shaders/CompositeSSAO.frag"),
			Rect{ viewPortX, viewPortY },
			Rect{ viewPortX, viewPortY }
		);

		GraphicsTask compositeTask("LightingComposite", desc);
		compositeTask.addInput(usingGlobalLighting ? kGlobalLighting : kAnalyticLighting, AttachmentType::Texture2D);
		compositeTask.addInput(kSSAO, AttachmentType::Texture2D);
		compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);
		compositeTask.addOutput(kCompositeOutput, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Swapchain, LoadOp::Clear_Black);
		compositeTask.addDrawCall(0, 3);

		graph.addTask(compositeTask);

		// Now add the overlay the final TAA'd result to the framebuffer.
		GraphicsPipelineDescription overlayDesc
		(
			eng->getShader("./Shaders/FullScreenTriangle.vert"),
			eng->getShader("./Shaders/CompositeOverlay.frag"),
			Rect{ viewPortX, viewPortY },
			Rect{ viewPortX, viewPortY }
		);

		GraphicsTask overlayTask("OverlayComposite", overlayDesc);
		overlayTask.addInput(kNewTAAHistory, AttachmentType::Texture2D);
		overlayTask.addInput(kOverlay, AttachmentType::Texture2D);
		overlayTask.addInput(kDefaultSampler, AttachmentType::Sampler);
		overlayTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Custom, LoadOp::Clear_Black);
		overlayTask.addDrawCall(0, 3);

		graph.addTask(overlayTask);
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
}


void CompositeTechnique::bindResources(RenderGraph& graph)
{
	graph.bindImage(kFrameBufer, getDevice()->getSwapChainImageView());
}
