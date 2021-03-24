#ifndef FORWARD_IBL_TECHNIQUE_HPP
#define FORWARD_IBL_TECHNIQUE_HPP

#include "RenderGraph/GraphicsTask.hpp"

#include "Technique.hpp"


class ForwardIBLTechnique : public Technique
{
public:

	ForwardIBLTechnique(RenderEngine*, RenderGraph&);
	~ForwardIBLTechnique() = default;

	virtual PassType getPassType() const
	{
		return PassType::ForwardIBL;
	}

    virtual void render(RenderGraph&, RenderEngine*) override final {};

    virtual void bindResources(RenderGraph&) override final {}

    virtual void postGraphCompilation(RenderGraph&, RenderEngine*) override final;

private:

    std::unordered_map<uint64_t, uint64_t> mMaterialPipelineVariants;
	GraphicsPipelineDescription mDesc;

	TaskID mTaskID;
};

#endif
