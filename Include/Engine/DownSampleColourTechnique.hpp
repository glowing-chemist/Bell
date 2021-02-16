#ifndef DOWN_SAMPLE_COLOUR_HPP
#define DOWN_SAMPLE_COLOUR_HPP

#include "Technique.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"


class DownSampleColourTechnique : public Technique
{
public:

	DownSampleColourTechnique(RenderEngine*, RenderGraph&);
	~DownSampleColourTechnique() = default;

	virtual PassType getPassType() const
	{
		return PassType::DownSampleColour;
	}

    virtual void render(RenderGraph&, RenderEngine*) override final;

	virtual void bindResources(RenderGraph& graph) override final;

	private:

    bool mFirstFrame;

    Image mDowmSampledColour[2];
    ImageView mDownSampledColourViews[7];

	Image mDowmSampledColourInternal;
	ImageView mDownSampledColourInternalViews[6];
};

#endif
