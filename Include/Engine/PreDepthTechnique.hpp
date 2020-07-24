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
    PreDepthTechnique(Engine* dev, RenderGraph&);
    virtual ~PreDepthTechnique() = default;

    virtual PassType getPassType() const final override
    { return PassType::DepthPre; }

    std::string getDepthName() const
    { return kGBufferDepth; }

    virtual void bindResources(RenderGraph& graph) override final {}

    virtual void render(RenderGraph&, Engine*) override final {}


private:

    GraphicsPipelineDescription mPipelineDescription;
    TaskID mTaskID;
};


#endif
