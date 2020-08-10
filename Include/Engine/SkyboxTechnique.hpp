#ifndef SKYBOX_TECHNIQUE_HPP
#define SKYBOX_TECHNIQUE_HPP

#include "Technique.hpp"

#include "RenderGraph/GraphicsTask.hpp"


class Engine;


class SkyboxTechnique : public Technique
{
public:
    SkyboxTechnique(Engine*, RenderGraph&);
    virtual ~SkyboxTechnique() = default;

    virtual PassType getPassType() const override final
    { return PassType::Skybox; }

    // default empty implementations as most classes won't need to do anything for one of these.
    virtual void render(RenderGraph&, Engine*) override final
    {}

    virtual void bindResources(RenderGraph&) override final
    {}

private:
    GraphicsPipelineDescription mPipelineDesc;

    Shader mSkyboxVertexShader;
    Shader mSkyboxFragmentShader;

    TaskID mTaskID;
};

#endif
