#ifndef PATH_TRACING_TECHNIQUE_HPP
#define PATH_TRACING_TECHNIQUE_HPP

#include "Technique.hpp"

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"

#include "Engine/DefaultResourceSlots.hpp"


class PathTracingTechnique : public Technique
{
public:
    PathTracingTechnique(Engine*, RenderGraph&);
    virtual ~PathTracingTechnique() = default;

    virtual PassType getPassType() const final override
    { return PassType::PathTracing; }

    virtual void bindResources(RenderGraph& graph) override final
    {
	if(!graph.isResourceSlotBound(kGlobalLighting))
	    graph.bindImage(kGlobalLighting, mGlobalLightingView);
    }
    virtual void render(RenderGraph&, Engine*) override final
    {
	mGloballighting->updateLastAccessed();
	mGlobalLightingView->updateLastAccessed();
    }

private:

    Image mGloballighting;
    ImageView mGlobalLightingView;

    ComputePipelineDescription mPipelineDescription;
    TaskID mTaskID;
};

#endif
