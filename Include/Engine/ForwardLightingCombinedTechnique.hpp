#ifndef FORWARD_COMBINED_LIGHTING_HPP
#define FORWARD_COMBINED_LIGHTING_HPP

#include "RenderGraph/GraphicsTask.hpp"

#include "Engine/Technique.hpp"


class ForwardCombinedLightingTechnique : public Technique
{
public:

	ForwardCombinedLightingTechnique(Engine*);
	~ForwardCombinedLightingTechnique() = default;

	virtual PassType getPassType() const
	{
		return PassType::ForwardCombinedLighting;
	}

	virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance*>&);

	virtual void bindResources(RenderGraph& graph)
	{
		graph.bindSampler("ForwardPointSampler", mPointSampler);
	}

private:

	GraphicsPipelineDescription mDesc;
	GraphicsTask mTask;

	Sampler mPointSampler;
};

#endif