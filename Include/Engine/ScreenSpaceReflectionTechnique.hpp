#ifndef SCREEN_SAPCE_REFLECTION_TECHNIQUE_HPP
#define SCREEN_SAPCE_REFLECTION_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "Core/PerFrameResource.hpp"

class Engine;
class RenderGraph;

class ScreenSpaceReflectionTechnique : public Technique
{
public:

	ScreenSpaceReflectionTechnique(Engine*, RenderGraph&);
	~ScreenSpaceReflectionTechnique() = default;

	virtual PassType getPassType() const override final
	{
		return PassType::SSR;
	}

	virtual void render(RenderGraph&, Engine*) override final {}

	virtual void bindResources(RenderGraph& graph) override final;

private:
    Image mReflectionMap;
    ImageView mReflectionMapView;

    Sampler mClampedSampler;
};

#endif
