#ifndef PREDEPTH_HPP
#define PREDEPTH_HPP

#include "Core/Image.hpp"
#include "Engine/Technique.hpp"
#include "Engine/Engine.hpp"

#include <string>


class PreDepthTechnique : public Technique<GraphicsTask>
{
public:
    PreDepthTechnique(Engine* dev);
    virtual ~PreDepthTechnique() = default;

    virtual PassType getPassType() const final override
    { return PassType::DepthPre; }

    virtual GraphicsTask& getTaskToRecord() override final;

    Image& getDepthImage()
    { return mDepthImage; }

    ImageView& getDepthView()
    { return mDepthView; }

    std::string getDepthName() const
    { return "PreDepth"; }


private:

    Image mDepthImage;
    ImageView mDepthView;

    GraphicsPipelineDescription mPipelineDescription;
    GraphicsTask mTask;
    bool mTaskInitialised;
};


#endif
