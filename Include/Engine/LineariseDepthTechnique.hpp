#ifndef LINEARISE_DEPTH_HPP
#define LINEARISE_DEPTH_HPP

#include "Engine/Technique.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/PerFrameResource.hpp"

class Engine;
class RenderGraph;

class LineariseDepthTechnique : public Technique
{
public:
	LineariseDepthTechnique(Engine*, RenderGraph&);

	virtual PassType getPassType() const override final
	{
		return PassType::LineariseDepth;
	}

	virtual void render(RenderGraph&, Engine*) override final;

	virtual void bindResources(RenderGraph& graph)
	{
		graph.bindImage(kLinearDepth, *mLinearDepthView);
	}

private:

	ComputePipelineDescription mPipelienDesc;
	TaskID mTaskID;

	PerFrameResource<Image> mLinearDepth;
	PerFrameResource<ImageView> mLinearDepthView;
};

#endif