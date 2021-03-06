#ifndef IMAGEBASEDLIGHTING_TECHNIQUE_HPP
#define IMAGEBASEDLIGHTING_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"


class DeferredImageBasedLightingTechnique : public Technique
{
public:

    DeferredImageBasedLightingTechnique(RenderEngine*, RenderGraph&);
    ~DeferredImageBasedLightingTechnique() = default;

    virtual PassType getPassType() const override final
    {
        return PassType::DeferredPBRIBL;
    }

    virtual void render(RenderGraph&, RenderEngine*) override final
    {}

    virtual void bindResources(RenderGraph&) override final {}

private:

    GraphicsPipelineDescription mPipelineDesc;

    Shader mIBLVertexShader;
    Shader mIBLFragmentShader;

    TaskID mTaskID;
};


#endif
