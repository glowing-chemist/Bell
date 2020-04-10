#ifndef ANALYTICALIMAGEBASEDLIGHTING_TECHNIQUE_HPP
#define ANALYTICALIMAGEBASEDLIGHTING_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"


class AnalyticalImageBasedLightingTechnique : public Technique
{
public:

    AnalyticalImageBasedLightingTechnique(Engine*, RenderGraph&);
    virtual ~AnalyticalImageBasedLightingTechnique() = default;

    virtual PassType getPassType() const override final
    {
		return PassType::DeferredTextureAnalyticalPBRIBL;
    }

    // default empty implementations as most classes won't need to do anything for one of these.
    virtual void render(RenderGraph& graph, Engine*) override final
    {}

    virtual void bindResources(RenderGraph&) override final {}

private:

    GraphicsPipelineDescription mPipelineDesc;
    TaskID mTaskID;
};

#endif
