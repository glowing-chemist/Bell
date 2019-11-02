#ifndef ANALYTICALIMAGEBASEDLIGHTING_TECHNIQUE_HPP
#define ANALYTICALIMAGEBASEDLIGHTING_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"


class AnalyticalImageBasedLightingTechnique : public Technique
{
public:

    AnalyticalImageBasedLightingTechnique(Engine*);
    virtual ~AnalyticalImageBasedLightingTechnique() = default;

    virtual PassType getPassType() const override final
    {
	return PassType::DeferredTextureAnalyticalPBRIBL;
    }

    // default empty implementations as most classes won't need to do anything for one of these.
    virtual void render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&) override final
    {
	graph.addTask(mTask);
    }

    virtual void bindResources(RenderGraph&) const override final {}

private:

    GraphicsPipelineDescription mPipelineDesc;
    GraphicsTask mTask;
};

#endif
