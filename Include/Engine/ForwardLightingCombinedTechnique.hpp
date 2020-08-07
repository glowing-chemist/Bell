#ifndef FORWARD_COMBINED_LIGHTING_HPP
#define FORWARD_COMBINED_LIGHTING_HPP

#include "RenderGraph/GraphicsTask.hpp"

#include "Engine/Technique.hpp"


class ForwardCombinedLightingTechnique : public Technique
{
public:

	ForwardCombinedLightingTechnique(Engine*, RenderGraph&);
	~ForwardCombinedLightingTechnique() = default;

	virtual PassType getPassType() const
	{
		return PassType::ForwardCombinedLighting;
	}

    virtual void render(RenderGraph&, Engine*) {}

	virtual void bindResources(RenderGraph& graph);

private:

	GraphicsPipelineDescription mDesc;

    Shader mForwardCombinedVertexShader;
    Shader mForwardCombinedFragmentShader;

	TaskID mTaskID;

	Sampler mPointSampler;
};

#endif
