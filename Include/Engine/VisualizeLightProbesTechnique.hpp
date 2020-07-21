#ifndef VISUALIZE_LIGHT_PROBES_HPP
#define VISUALIZE_LIGHT_PROBES_HPP

#include "Engine/Technique.hpp"

extern const char kLightProbeVertexBuffer[];
extern const char kLightProbeIndexBuffer[];


class ViaualizeLightProbesTechnique : public Technique
{

public:

    ViaualizeLightProbesTechnique(Engine*, RenderGraph&);
    ~ViaualizeLightProbesTechnique() = default;

    virtual PassType getPassType() const
    {
    return PassType::VisualizeLightProbes;
    }

    virtual void render(RenderGraph&, Engine*) {}

    virtual void bindResources(RenderGraph&) {}

private:

    GraphicsPipelineDescription mIrradianceProbePipeline;
    GraphicsPipelineDescription mIrradianceVolumePipeline;
    TaskID mTaskID;

    Buffer mVertexBuffer;
    Buffer mIndexBuffer;
    uint32_t mIndexCount;
};

#endif