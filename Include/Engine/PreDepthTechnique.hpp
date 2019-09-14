#ifndef PREDEPTH_HPP
#define PREDEPTH_HPP

#include "Core/Image.hpp"
#include "Engine/Technique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


#include <string>


class PreDepthTechnique : public Technique<GraphicsTask>
{
public:
    PreDepthTechnique(Engine* dev);
    virtual ~PreDepthTechnique() = default;

    virtual PassType getPassType() const final override
    { return PassType::DepthPre; }

    virtual GraphicsTask& getTask() final override
    { return mTask; }

    Image& getDepthImage()
    { return mDepthImage; }

    ImageView& getDepthView()
    { return mDepthView; }

    std::string getDepthName() const
    { return kPreDepth; }

    virtual void bindResources(RenderGraph& graph) const override final
    { graph.bindImage(getDepthName(), mDepthView); }


private:

    Image mDepthImage;
    ImageView mDepthView;

    GraphicsPipelineDescription mPipelineDescription;
    GraphicsTask mTask;
};


#endif
