#ifndef CONVOLVERSKYBOX_TECHNIQUE_HPPP
#define CONVOLVERSKYBOX_TECHNIQUE_HPPP

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "Technique.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "RenderGraph/ComputeTask.hpp"


class ConvolveSkyBoxTechnique : public Technique
{
public:

	ConvolveSkyBoxTechnique(Engine*, RenderGraph&);

	virtual PassType getPassType() const override final
	{
		return PassType::ConvolveSkybox;
	}

	// default empty implementations as most classes won't need to do anything for one of these.
	virtual void render(RenderGraph&, Engine*) override final;

    virtual void bindResources(RenderGraph& graph) override final;

private:
	ComputePipelineDescription mPipelineDesc;
	TaskID mTaskID;
    Image mConvolvedSpecularSkybox;
    ImageView mConvolvedSpecularView;
    Image mConvolvedDiffuseSkybox;
    ImageView mConvolvedDiffuseView;

    std::vector<ImageView> mConvolvedMips;

	bool mFirstFrame;
};

#endif
