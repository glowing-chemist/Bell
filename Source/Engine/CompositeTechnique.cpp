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
    const bool usingSSAO = eng->isPassRegistered(PassType::SSAO);
	const bool usingOverlay = eng->isPassRegistered(PassType::Overlay);
	const bool usingTAA = eng->isPassRegistered(PassType::TAA);
    const bool usingSSR = eng->isPassRegistered(PassType::SSR) || eng->isPassRegistered(PassType::RayTracedReflections);

    if (eng->debugTextureEnabled())
	{
		GraphicsPipelineDescription desc
		(
            Rect{ viewPortX, viewPortY },
			Rect{ viewPortX, viewPortY }
		);

		GraphicsTask compositeTask("Composite", desc);
        compositeTask.addInput(eng->getDebugTextureSlot(), AttachmentType::Texture2D);
		compositeTask.addInput(kOverlay, AttachmentType::Texture2D);
		compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

		compositeTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), LoadOp::Clear_Black);

		compositeTask.setRecordCommandsCallback(
            [](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
			{
                Shader vertexShader = eng->getShader("./Shaders/FullScreenTriangle.vert");
                Shader fragmentShader = eng->getShader("./Shaders/CompositeOverlay.frag");

                const RenderTask& task = graph.getTask(taskIndex);
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, vertexShader, nullptr, nullptr, nullptr, fragmentShader);

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

            if (usingSSR && !usingTAA)
            {
                compositeTask.addInput(kReflectionMap, AttachmentType::Texture2D);
                compositeTask.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
                compositeTask.addInput(kReflecionUVs, AttachmentType::Texture2D);

            }

            compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

            if(usingTAA)
                compositeTask.addManagedOutput(kCompositeOutput, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), SizeClass::Swapchain, LoadOp::Nothing);
            else
                compositeTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), LoadOp::Nothing);


            compositeTask.setRecordCommandsCallback(
                [](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    Shader vertexShader = eng->getShader("./Shaders/FullScreenTriangle.vert");
                    Shader fragmentShader = eng->getShader("./Shaders/FinalComposite.frag");

                    const RenderTask& task = graph.getTask(taskIndex);
                    exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, vertexShader, nullptr, nullptr, nullptr, fragmentShader);

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
                Rect{ viewPortX, viewPortY },
                Rect{ viewPortX, viewPortY }
            );

            GraphicsTask overlayTask("OverlayComposite", overlayDesc);
            overlayTask.addInput(kNewTAAHistory, AttachmentType::Texture2D);

            if(eng->isPassRegistered(PassType::Overlay))
                overlayTask.addInput(kOverlay, AttachmentType::Texture2D);

            if (usingSSR)
            {
                overlayTask.addInput(kReflectionMap, AttachmentType::Texture2D);
                overlayTask.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
                overlayTask.addInput(kReflecionUVs, AttachmentType::Texture2D);
            }

            overlayTask.addInput(kDefaultSampler, AttachmentType::Sampler);
            overlayTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
            overlayTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), LoadOp::Clear_Black);
            overlayTask.setRecordCommandsCallback(
                [](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    Shader vertexShader = eng->getShader("./Shaders/FullScreenTriangle.vert");
                    Shader fragmentShader = eng->getShader("./Shaders/CompositeOverlayTAA.frag");

                    const RenderTask& task = graph.getTask(taskIndex);
                    exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, vertexShader, nullptr, nullptr, nullptr, fragmentShader);

                    exec->draw(0, 3);
                }
            );

            graph.addTask(overlayTask);
        }

        if (usingOverlay && !usingGlobalLighting && !usingAnalyticalLighting)
        {
            GraphicsPipelineDescription desc
            (
                Rect{ viewPortX, viewPortY },
                Rect{ viewPortX, viewPortY }
            );

            GraphicsTask compositeTask("Composite", desc);
            compositeTask.addInput(kOverlay, AttachmentType::Texture2D);
            compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

            compositeTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage()->getFormat(), LoadOp::Clear_Black);

            compositeTask.setRecordCommandsCallback(
                [](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                {
                    Shader vertexShader = eng->getShader("./Shaders/FullScreenTriangle.vert");
                    Shader fragmentShader = eng->getShader("./Shaders/Blit.frag");

                    const RenderTask& task = graph.getTask(taskIndex);
                    exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, vertexShader, nullptr, nullptr, nullptr, fragmentShader);

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
