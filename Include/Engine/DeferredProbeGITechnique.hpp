#ifndef DEFERRED_PROBE_GI_TECHNIQUE_HPP
#define DEFERRED_PROBE_GI_TECHNIQUE_HPP

#include "Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"

class DeferredProbeGITechnique : public Technique
{
public:
    DeferredProbeGITechnique(Engine*, RenderGraph&);
    ~DeferredProbeGITechnique() = default;

    virtual PassType getPassType() const override final
    {
	return PassType::LightProbeDeferredGI;
    }

    virtual void render(RenderGraph&, Engine*) override final
    {}

    virtual void bindResources(RenderGraph&) override final {}

private:
    GraphicsPipelineDescription mPipelineDesc;
    Shader mProbeVertexShader;
    Shader mProbeFragmentShader;
    TaskID mTaskID;
};

#endif
