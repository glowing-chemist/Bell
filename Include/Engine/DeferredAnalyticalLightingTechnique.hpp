#ifndef DEFERRED_ANALYTICAL_LIGHTING_TECHNIQUE
#define DEFERRED_ANALYTICAL_LIGHTING_TECHNIQUE


#include "Engine/Technique.hpp"
#include "Core/PerFrameResource.hpp"
#include "Engine/DefaultResourceSlots.hpp"


class DeferredAnalyticalLightingTechnique : public Technique
{
public:
	DeferredAnalyticalLightingTechnique(RenderEngine*, RenderGraph&);
	~DeferredAnalyticalLightingTechnique() = default;

	virtual PassType getPassType() const override final
	{
		return PassType::DeferredAnalyticalLighting;
	}

	virtual void render(RenderGraph& graph, RenderEngine*) override final
	{
        mAnalyticalLighting->updateLastAccessed();
        mAnalyticalLightingView->updateLastAccessed();
	}

	virtual void bindResources(RenderGraph& graph) override final;

private:

    Shader mDeferredAnalitucalLightingShader;
	TaskID mTaskID;

    Image mAnalyticalLighting;
    ImageView mAnalyticalLightingView;
};

#endif
