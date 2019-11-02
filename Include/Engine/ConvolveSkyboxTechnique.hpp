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

	ConvolveSkyBoxTechnique(Engine*);

	virtual PassType getPassType() const override final
	{
		return PassType::ConvolveSkybox;
	}

	// default empty implementations as most classes won't need to do anything for one of these.
	virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance*>&) override final;

	virtual void bindResources(RenderGraph& graph) const override final
	{
		graph.bindImage(kConvolvedSkyBox, mConvolvedView);
	}

private:
	ComputePipelineDescription mPipelineDesc;
	Image mConvolvedSkybox;
	ImageView mConvolvedView;
	bool mFirstFrame;
};

#endif
