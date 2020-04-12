#include "Engine/ScreenSpaceReflectionTechnique.hpp"
#include "Engine/Engine.hpp"


ScreenSpaceReflectionTechnique::ScreenSpaceReflectionTechnique(Engine* eng, RenderGraph& graph) :
	Technique("SSR", eng->getDevice()),
	mReflectionMap(eng->getDevice(), Format::RGBA8UNorm, eng->getSwapChainImage()->getExtent(0, 0).width, eng->getSwapChainImage()->getExtent(0, 0).height, 1, 1, 1, "Reflection map"),
	mReflectionMapView(mReflectionMap, ImageViewType::Colour)
{
	const auto viewPortX = eng->getSwapChainImage()->getExtent(0, 0).width;
	const auto viewPortY = eng->getSwapChainImage()->getExtent(0, 0).height;
	GraphicsPipelineDescription desc
	(
		eng->getShader("./Shaders/FullScreenTriangle.vert"),
		eng->getShader("./Shaders/ScreenSpaceReflection.frag"),
		Rect{ viewPortX, viewPortY },
		Rect{ viewPortX, viewPortY }
	);

	GraphicsTask compositeTask("SSR", desc);
}