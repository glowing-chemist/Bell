#include "Engine/CompositeTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "Core/Executor.hpp"


CompositeTechnique::CompositeTechnique(Engine* eng, RenderGraph& graph) :
	Technique("Composite", eng->getDevice()) 
{
	const auto viewPortX = eng->getSwapChainImage()->getExtent(0, 0).width;
	const auto viewPortY = eng->getSwapChainImage()->getExtent(0, 0).height;

    const bool usingGlobalLighting = eng->isPassRegistered(PassType::ForwardIBL) || eng->isPassRegistered(PassType::DeferredPBRIBL)
        || eng->isPassRegistered(PassType::ForwardCombinedLighting) || eng->isPassRegistered(PassType::LightProbeDeferredGI) || eng->isPassRegistered(PassType::PathTracing);
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

		compositeTask.setRecordCommandsCallback(
            [](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
			{
				exec->draw(0, 3);
			}
		);

		graph.addTask(compositeTask);
	}
    else
    {
        if(usingAnalyticalLighting || usingGlobalLighting)
        {
            GraphicsPipelineDescription desc
            {
                eng->getShader("./Shaders/FullScreenTriangle.vert"),
                eng->getShader("./Shaders/FinalComposite.frag"),
                Rect{ viewPortX, viewPortY },
                Rect{ viewPortX, viewPortY }
            };

            GraphicsTask compositeTask("Composite", desc);
            compositeTask.addInput(kGlobalLighting, AttachmentType::Texture2D);
            if (usingAnalyticalLighting)
                compositeTask.addInput(kAnalyticLighting, AttachmentType::Texture2D);
            if (usingOverlay)
                compositeTask.addInput(kOverlay, AttachmentType::Texture2D);
            if (usingSSAO)
                compositeTask.addInput(kSSAO, AttachmentType::Texture2D);
            compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

            compositeTask.addOutput(usingTAA ? kCompositeOutput : kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), usingTAA ? SizeClass::Swapchain : SizeClass::Custom, LoadOp::Nothing);

            compositeTask.setRecordCommandsCallback(
                [](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
                {
                    exec->draw(0, 3);
                }
            );

            graph.addTask(compositeTask);
        }

        if (usingTAA)
        {
            // Now add the overlay the final TAA'd result to the framebuffer.
            GraphicsPipelineDescription overlayDesc
            (
                eng->getShader("./Shaders/FullScreenTriangle.vert"),
                eng->getShader("./Shaders/CompositeOverlayTAA.frag"),
                Rect{ viewPortX, viewPortY },
                Rect{ viewPortX, viewPortY }
            );

            GraphicsTask overlayTask("OverlayComposite", overlayDesc);
            overlayTask.addInput(kNewTAAHistory, AttachmentType::Texture2D);

            if(eng->isPassRegistered(PassType::Overlay))
                overlayTask.addInput(kOverlay, AttachmentType::Texture2D);

            overlayTask.addInput(kDefaultSampler, AttachmentType::Sampler);
            overlayTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
            if(eng->isPassRegistered(PassType::SSR))
                overlayTask.addInput(kReflectionMap, AttachmentType::Texture2D);
            overlayTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Custom, LoadOp::Clear_Black);
            overlayTask.setRecordCommandsCallback(
                [](Executor* exec, Engine*, const std::vector<const MeshInstance*>&)
                {
                    exec->draw(0, 3);
                }
            );

            graph.addTask(overlayTask);
        }

        if (usingOverlay && !usingGlobalLighting && !usingAnalyticalLighting)
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

            compositeTask.setRecordCommandsCallback(
                [](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    exec->draw(0, 3);
                }
            );

            graph.addTask(compositeTask);
        }
    }
}


void CompositeTechnique::bindResources(RenderGraph& graph)
{
	graph.bindImage(kFrameBufer, getDevice()->getSwapChainImageView());
}
