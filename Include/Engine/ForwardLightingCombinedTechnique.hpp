#ifndef FORWARD_COMBINED_LIGHTING_HPP
#define FORWARD_COMBINED_LIGHTING_HPP

#include "RenderGraph/GraphicsTask.hpp"

#include "Engine/Technique.hpp"


class ForwardCombinedLightingTechnique : public Technique
{
public:

	ForwardCombinedLightingTechnique(RenderEngine*, RenderGraph&);
	~ForwardCombinedLightingTechnique() = default;

	virtual PassType getPassType() const
	{
		return PassType::ForwardCombinedLighting;
	}

    virtual void render(RenderGraph&, RenderEngine*) override final {}

	virtual void bindResources(RenderGraph& graph) override final;

    virtual void postGraphCompilation(RenderGraph&, RenderEngine*) override final;

private:

	GraphicsPipelineDescription mDesc;

    std::unordered_map<uint64_t, uint64_t> mMaterialPipelineVariants;

	TaskID mTaskID;

	Sampler mPointSampler;
};

#endif
