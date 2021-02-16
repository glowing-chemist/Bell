#ifndef PREDEPTH_HPP
#define PREDEPTH_HPP

#include "Core/Image.hpp"
#include "Engine/Technique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


#include <string>


class PreDepthTechnique : public Technique
{
public:
    PreDepthTechnique(RenderEngine* dev, RenderGraph&);
    virtual ~PreDepthTechnique() = default;

    virtual PassType getPassType() const final override
    { return PassType::DepthPre; }

    std::string getDepthName() const
    { return kGBufferDepth; }

    virtual void bindResources(RenderGraph&) override final {}

    virtual void render(RenderGraph&, RenderEngine*) override final {}


private:

    GraphicsPipelineDescription mPipelineDescription;

    Shader mPreDepthVertexShader;
    Shader mPreDepthFragmentShader;

    TaskID mTaskID;
};


#endif
