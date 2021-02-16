#ifndef COMPOSITE_TECHNIQUE_HPP
#define COMPOSITE_TECHNIQUE_HPP

#include "Technique.hpp"
#include "Engine/DefaultResourceSlots.hpp"


class CompositeTechnique : public Technique
{
public:

	CompositeTechnique(RenderEngine*, RenderGraph&);
	virtual ~CompositeTechnique() = default;

	virtual PassType getPassType() const
	{
		return PassType::Composite;
	}

	// default empty implementations as most classes won't need to do anything for one of these.
	virtual void render(RenderGraph&, RenderEngine*) override final
	{}

    virtual void bindResources(RenderGraph& graph) override final;
};

#endif
