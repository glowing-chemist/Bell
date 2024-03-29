#ifndef LINEARISE_DEPTH_HPP
#define LINEARISE_DEPTH_HPP

#include "Engine/Technique.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/PerFrameResource.hpp"

class RenderEngine;
class RenderGraph;

class LineariseDepthTechnique : public Technique
{
public:
	LineariseDepthTechnique(RenderEngine*, RenderGraph&);

	virtual PassType getPassType() const override final
	{
		return PassType::LineariseDepth;
	}

	virtual void render(RenderGraph&, RenderEngine*) override final;

    virtual void bindResources(RenderGraph& graph) override final;

private:

    uint32_t mMipLevels;
    Shader mLineariseDepthShader;
	TaskID mTaskID;

    Image mLinearDepth;
    ImageView mLinearDepthView;
    Image mPreviousLinearDepth;
    ImageView mPreviousLinearDepthView;

    std::vector<ImageView> mMipsViews;
    std::vector<ImageView> mPrevMipsViews;

    bool mFirstFrame;

    Sampler mOcclusionSampler;
};

#endif
