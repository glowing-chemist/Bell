#ifndef SSAOTECHNIQUE_HPP
#define SSAOTECHNIQUE_HPP

#include "Core/Image.hpp"
#include "Engine/Technique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include <string>


class SSAOTechnique : public Technique<GraphicsTask>
{
public:
	SSAOTechnique(Engine* dev);
	virtual ~SSAOTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::SSAO; }

    virtual GraphicsTask& getTask() final override
    { return mTask; }

	void setDepthName(const std::string& depthSlot)
	{ mDepthNameSlot = depthSlot; }

	Image& getSSAOImage()
		{ return mSSAOImage; }

	std::string getSSAOImageName() const
        { return kSSAO; }

	Sampler& getSampler()
		{ return mLinearSampler; }

	std::string getSSAOSamplerName() const
        { return kDefaultSampler; }

    virtual void bindResources(RenderGraph&) const override final;

private:

	std::string mDepthNameSlot;

	Image mSSAOImage;
	ImageView mSSAOIMageView;

	Sampler mLinearSampler;

	GraphicsPipelineDescription mPipelineDesc;
	GraphicsTask mTask;
};


#endif
