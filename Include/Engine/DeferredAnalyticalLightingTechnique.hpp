#ifndef DEFERRED_ANALYTICAL_LIGHTING_TECHNIQUE
#define DEFERRED_ANALYTICAL_LIGHTING_TECHNIQUE


#include "Engine/Technique.hpp"
#include "Core/PerFrameResource.hpp"
#include "Engine/DefaultResourceSlots.hpp"


class DeferredAnalyticalLightingTechnique : public Technique
{
public:
	DeferredAnalyticalLightingTechnique(Engine*, RenderGraph&);
	~DeferredAnalyticalLightingTechnique() = default;

	virtual PassType getPassType() const override final
	{
		return PassType::DeferredAnalyticalLighting;
	}

	virtual void render(RenderGraph& graph, Engine*) override final
	{
        mAnalyticalLighting->updateLastAccessed();
        mAnalyticalLightingView->updateLastAccessed();
	}

    virtual void bindResources(RenderGraph& graph) override final
	{
        graph.bindImage(kAnalyticLighting, mAnalyticalLightingView);
		graph.bindSampler("PointSampler", mPointSampler);
	}

private:

	ComputePipelineDescription mPipelineDesc;
	TaskID mTaskID;

    Image mAnalyticalLighting;
    ImageView mAnalyticalLightingView;

	Sampler mPointSampler;
};

#endif
