#ifndef DEFERRED_ANALYTICAL_LIGHTING_TECHNIQUE
#define DEFERRED_ANALYTICAL_LIGHTING_TECHNIQUE


#include "Engine/Technique.hpp"
#include "Engine/DefaultResourceSlots.hpp"


class DeferredAnalyticalLightingTechnique : public Technique
{
public:
	DeferredAnalyticalLightingTechnique(Engine*);
	~DeferredAnalyticalLightingTechnique() = default;

	virtual PassType getPassType() const override final
	{
		return PassType::DeferredAnalyticalLighting;
	}

	virtual void render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&) override final
	{
		graph.addTask(mTask);
	}

	virtual void bindResources(RenderGraph& graph) const override final
	{
		graph.bindImage(kAnalyticLighting, mAnalyticalLightingView);
	}

private:

	ComputePipelineDescription mPipelineDesc;
	ComputeTask mTask;

	Image mAnalyticalLighting;
	ImageView mAnalyticalLightingView;
};

#endif