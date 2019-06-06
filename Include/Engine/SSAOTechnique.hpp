#ifndef SSAOTECHNIQUE_HPP
#define SSAOTECHNIQUE_HPP

#include "Core/Image.hpp"
#include "Engine/Technique.hpp"
#include "Engine/Engine.hpp"

#include <string>


class SSAOTechnique : public Technique
{
public:
	SSAOTechnique(Engine* dev);

	virtual PassType getPassType() const final override
		{ return PassType::SSAO; }

	virtual RenderTask& getTaskToRecord() override final;

	void setDepthSlot(const std::string& depthSlot)
	{ mDepthNameSlot = depthSlot; }

	Image& getSSAOImage()
		{ return mSSAOImage; }

	Sampler& getSampler()
		{ return mLinearSampler; }

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
