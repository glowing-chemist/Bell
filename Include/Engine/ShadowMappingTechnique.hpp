#ifndef SHADOW_MAPPING_HPP
#define SHADOW_MAPPING_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "Engine/DefaultResourceSlots.hpp"

static constexpr char kShadowMapRaw[] = "ShadowMapRaw";
static constexpr char kShadowMapBlurIntermediate[] = "ShadowMapBlurIntermediate";
static constexpr char kShadowMapBlured[] = "ShadowMapBlured";

class ShadowMappingTechnique : public Technique
{
public:
    ShadowMappingTechnique(Engine*, RenderGraph&);
    ~ShadowMappingTechnique() = default;

    virtual PassType getPassType() const override
    {
        return PassType::Shadow;
    }

    virtual void render(RenderGraph&, Engine*) override;

    virtual void bindResources(RenderGraph& graph) override 
    {
        graph.bindImage(kShadowMap, mShadowMapView);
        graph.bindImage(kShadowMapBlurIntermediate, mShadowMapIntermediateView);
        graph.bindImage(kShadowMapBlured, mShadowMapBluredView);
    }

private:
    GraphicsPipelineDescription mDesc;
    TaskID                      mShadowTask;

    ComputePipelineDescription mBlurXDesc;
    TaskID                     mBlurXTaskID;

    ComputePipelineDescription mBlurYDesc;
    TaskID                     mBlurYTaskID;

    ComputePipelineDescription mResolveDesc;
    TaskID                     mResolveTaskID;

    Image    mShadowMap;
    ImageView mShadowMapView;

    Image    mShadowMapIntermediate;
    ImageView mShadowMapIntermediateView;

    Image    mShadowMapBlured;
    ImageView mShadowMapBluredView;

};

#endif
