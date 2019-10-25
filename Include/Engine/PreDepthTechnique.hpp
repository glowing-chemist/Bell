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
    PreDepthTechnique(Engine* dev);
    virtual ~PreDepthTechnique() = default;

    virtual PassType getPassType() const final override
    { return PassType::DepthPre; }

    Image& getDepthImage()
    { return mDepthImage; }

    ImageView& getDepthView()
    { return mDepthView; }

    std::string getDepthName() const
    { return kPreDepth; }

    virtual void bindResources(RenderGraph& graph) const override final
    { graph.bindImage(getDepthName(), mDepthView); }
	virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance *> &) override final;


private:

    Image mDepthImage;
    ImageView mDepthView;

    GraphicsPipelineDescription mPipelineDescription;
    GraphicsTask mTask;
};


#endif
