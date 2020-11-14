#ifndef SHADOW_MAPPING_HPP
#define SHADOW_MAPPING_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "Engine/DefaultResourceSlots.hpp"

extern const char kShadowMapRaw[];
extern const char kShadowMapDepth[];
extern const char kShadowMapBlurIntermediate[];
extern const char kShadowMapBlured[];
extern const char kShadowMapUpsamped[];
extern const char kShadowMapHistory[];

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
        if(!graph.isResourceSlotBound(kShadowMap))
        {
            graph.bindImage(kShadowMapRaw, mShadowMapRawView);
            graph.bindImage(kShadowMapDepth, mShadowMapDepthView);
            graph.bindImage(kShadowMap, mShadowMapView);
            graph.bindImage(kShadowMapBlurIntermediate, mShadowMapIntermediateView);
            graph.bindImage(kShadowMapBlured, mShadowMapBluredView);
        }
    }

private:
    GraphicsPipelineDescription mDesc;
    TaskID                      mShadowTask;

    Shader mShadowMapVertexShader;
    Shader mShadowMapFragmentShader;


    Shader                     mBlurXShader;
    TaskID                     mBlurXTaskID;

    Shader                     mBlurYShader;
    TaskID                     mBlurYTaskID;

    Shader                     mResolveShader;
    TaskID                     mResolveTaskID;

    Image    mShadowMapDepth;
    ImageView mShadowMapDepthView;

    Image    mShadowMapRaw;
    ImageView mShadowMapRawView;

    Image    mShadowMap;
    ImageView mShadowMapView;

    Image    mShadowMapIntermediate;
    ImageView mShadowMapIntermediateView;

    Image    mShadowMapBlured;
    ImageView mShadowMapBluredView;

};


class RayTracedShadowsTechnique : public Technique
{
public:
    RayTracedShadowsTechnique(Engine*, RenderGraph&);
    ~RayTracedShadowsTechnique() = default;

    virtual PassType getPassType() const override
    {
        return PassType::RayTracedShadows;
    }

    virtual void render(RenderGraph&, Engine*) override {}

    virtual void bindResources(RenderGraph& graph) override;

private:

    Image    mShadowMapRaw;
    ImageView mShadowMapViewRaw;

    Image mShadowMapHistory;
    ImageView mShadowMapHistoryView;

    Image mShadowMapUpsampled;
    ImageView mShadowMapUpSampledView;

    Image    mShadowMap;
    ImageView mShadowMapView;

    Shader                     mRayTracedShadowsShader;
    TaskID                     mTaskID;
};

#endif
