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

    virtual void render(RenderGraph&, RenderEngine*) {};

    virtual void bindResources(RenderGraph&) {}

private:

	GraphicsPipelineDescription mDesc;

	TaskID mTaskID;
};

#endif
