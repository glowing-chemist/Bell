#include "Engine/CompositeTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "RenderGraph/GraphicsTask.hpp"


CompositeTechnique::CompositeTechnique(Engine* eng) :
	Technique("Composite", eng->getDevice()) {}


void CompositeTechnique::render(RenderGraph& graph, Engine* eng, const std::vector<const Scene::MeshInstance*>&)
{
	const auto viewPortX = eng->getSwapChainImage().getExtent(0, 0).width;
	const auto viewPortY = eng->getSwapChainImage().getExtent(0, 0).height;

	GraphicsPipelineDescription desc
	(
		eng->getShader("./Shaders/FullScreenTriangle.vert"),
		eng->getShader("./Shaders/CompositeOverlay.frag"),
		Rect{ viewPortX, viewPortY },
		Rect{ viewPortX, viewPortY }
	);

	GraphicsTask compositeTask("Composite", desc);
	compositeTask.addInput(kGlobalLighting, AttachmentType::Texture2D);
	compositeTask.addInput(kOverlay, AttachmentType::Texture2D);
	compositeTask.addInput(kDefaultSampler, AttachmentType::Sampler);

	compositeTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, eng->getSwapChainImage().getFormat(), SizeClass::Custom, LoadOp::Clear_Black);

	compositeTask.addDrawCall(0, 3);

	graph.addTask(compositeTask);
}