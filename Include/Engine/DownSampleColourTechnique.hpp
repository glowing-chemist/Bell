#ifndef DOWN_SAMPLE_COLOUR_HPP
#define DOWN_SAMPLE_COLOUR_HPP

#include "Technique.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"


class DownSampleColourTechnique : public Technique
{
public:

	DownSampleColourTechnique(Engine*, RenderGraph&);
	~DownSampleColourTechnique() = default;

	virtual PassType getPassType() const
	{
		return PassType::DownSampleColour;
	}

	virtual void render(RenderGraph&, Engine*) override final
	{}

	virtual void bindResources(RenderGraph& graph) override final;

	private:

	Image mDowmSampledColour;
	ImageView mDownSampledColourViews[6];

	Image mDowmSampledColourInternal;
	ImageView mDownSampledColourInternalViews[6];
};

#endif