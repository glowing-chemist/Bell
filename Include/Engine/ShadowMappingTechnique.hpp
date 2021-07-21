#ifndef SHADOW_MAPPING_HPP
#define SHADOW_MAPPING_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "Engine/DefaultResourceSlots.hpp"

extern const char kShadowMapRaw[];
extern const char kShadowMapCounter[];
extern const char kShadowMapDepth[];
extern const char kShadowMapBlurIntermediate[];
extern const char kShadowMapBlured[];
extern const char kShadowMapUpsamped[];
extern const char kShadowMapHistory[];

class ShadowMappingTechnique : public Technique
{
public:
    ShadowMappingTechnique(RenderEngine*, RenderGraph&);
    ~ShadowMappingTechnique() = default;

    virtual PassType getPassType() const override
    {
        return PassType::Shadow;
    }

    virtual void render(RenderGraph&, RenderEngine*) override;

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

    virtual void postGraphCompilation(RenderGraph&, RenderEngine*) override final;

private:
    GraphicsPipelineDescription mDesc;
    TaskID                      mShadowTask;

    uint64_t mShadowMappingPipelines[2];

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
    RayTracedShadowsTechnique(RenderEngine*, RenderGraph&);
    ~RayTracedShadowsTechnique() = default;

    virtual PassType getPassType() const override
    {
        return PassType::RayTracedShadows;
    }

    virtual void render(RenderGraph&, RenderEngine*) override
    {
        if(mFirstFrame)
        {
            mShadowMapCounter->clear(float4(0.0f, 0.0f, 0.0f, 0.0f));
            mShadowMapHistory[1]->clear(float4(0.0f, 0.0f, 0.0f, 0.0f));
            mFirstFrame = false;
        }

        mShadowMapCounter->updateLastAccessed();
        mShadowMapCounterView->updateLastAccessed();
        mShadowMap->updateLastAccessed();
        mShadowMapView->updateLastAccessed();
        for(uint32_t i = 0; i < 2; ++i)
        {
            mShadowMapHistory[i]->updateLastAccessed();
            mShadowMapHistoryView[i]->updateLastAccessed();
        }
    }

    virtual void bindResources(RenderGraph& graph) override;

private:

    bool mFirstFrame;

    Image    mShadowMapCounter;
    ImageView mShadowMapCounterView;

    Image mShadowMapHistory[2];
    ImageView mShadowMapHistoryView[2];

    Image    mShadowMap;
    ImageView mShadowMapView;

    Shader                     mRayTracedShadowsShader;
    TaskID                     mTaskID;
};

#endif
