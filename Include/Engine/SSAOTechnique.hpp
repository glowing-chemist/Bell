#ifndef SSAOTECHNIQUE_HPP
#define SSAOTECHNIQUE_HPP

#include "Core/Image.hpp"
#include "Engine/Technique.hpp"
#include "Engine/Engine.hpp"

#include <string>


class SSAOTechnique : public Technique<GraphicsTask>
{
public:
	SSAOTechnique(Engine* dev);
	virtual ~SSAOTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::SSAO; }

	virtual GraphicsTask& getTaskToRecord() override final;

	void setDepthSlot(const std::string& depthSlot)
	{ mDepthNameSlot = depthSlot; }

	Image& getSSAOImage()
		{ return mSSAOImage; }

	std::string getSSAOImageName() const
		{ return "SSAOImage"; }

	Sampler& getSampler()
		{ return mLinearSampler; }

	std::string getSSAOSamplerName() const
		{ return "LinearSampler"; }

private:

	std::string mDepthNameSlot;

	Image mSSAOImage;
	ImageView mSSAOIMageView;

	Sampler mLinearSampler;

	Shader mFullScreenTriangleVertex;
	Shader mSSAOFragmentShader;

	GraphicsPipelineDescription mPipelineDesc;
	GraphicsTask mTask;
};


#endif
