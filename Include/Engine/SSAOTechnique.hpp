#ifndef SSAOTECHNIQUE_HPP
#define SSAOTECHNIQUE_HPP

#include "Core/Image.hpp"
#include "Engine/Technique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include <string>


class SSAOTechnique : public Technique
{
public:
	SSAOTechnique(Engine* dev);
	virtual ~SSAOTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::SSAO; }

	void setDepthName(const std::string& depthSlot)
	{ mDepthNameSlot = depthSlot; }

	std::string getSSAOImageName() const
        { return kSSAO; }

	Sampler& getSampler()
		{ return mLinearSampler; }

	std::string getSSAOSamplerName() const
        { return kDefaultSampler; }

    virtual void bindResources(RenderGraph&) const override final;
	virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance *> &) override final;

private:

	std::string mDepthNameSlot;

	Sampler mLinearSampler;

	GraphicsPipelineDescription mPipelineDesc;
	GraphicsTask mTask;
};


#endif
