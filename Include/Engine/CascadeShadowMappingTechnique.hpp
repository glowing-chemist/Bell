#ifndef CASCADE_SHADOW_MAPPING_TECHNIQUE_HPP
#define CASCADE_SHADOW_MAPPING_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/PerFrameResource.hpp"

static constexpr char kCascadeShadowMapRaw0[] = "CascadeShadowMapRaw0";
static constexpr char kCascadeShadowMapRaw1[] = "CascadeShadowMapRaw1";
static constexpr char kCascadeShadowMapRaw2[] = "CascadeShadowMapRaw2";

static constexpr char kCascadeShadowMapBlurIntermediate0[] = "cascadeShadowMapBlurIntermediate0";
static constexpr char kCascadeShadowMapBlurIntermediate1[] = "cascadeShadowMapBlurIntermediate1";
static constexpr char kCascadeShadowMapBlurIntermediate2[] = "cascadeShadowMapBlurIntermediate2";

static constexpr char kCascadeShadowMapBlured0[] = "CascadeShadowMapBlured0";
static constexpr char kCascadeShadowMapBlured1[] = "CascadeShadowMapBlured1";
static constexpr char kCascadeShadowMapBlured2[] = "CascadeShadowMapBlured2";

static constexpr char kCascadesInfo[] = "CascadesInfo";


class CascadeShadowMappingTechnique : public Technique
{
public:
    CascadeShadowMappingTechnique(Engine*, RenderGraph&);
    ~CascadeShadowMappingTechnique() = default;

    virtual PassType getPassType() const override
    {
        return PassType::CascadingShadow;
    }

    virtual void render(RenderGraph&, Engine* eng) override;

    virtual void bindResources(RenderGraph& graph) override
    {
        if(!graph.isResourceSlotBound(kShadowMap))
        {
            graph.bindImage(kCascadeShadowMapRaw0, mCascadeShaowMapsViewMip0);
            graph.bindImage(kCascadeShadowMapRaw1, mCascadeShaowMapsViewMip1);
            graph.bindImage(kCascadeShadowMapRaw2, mCascadeShaowMapsViewMip2);

            graph.bindImage(kShadowMap, mResolvedShadowMapView);

            graph.bindImage(kCascadeShadowMapBlurIntermediate0, mIntermediateShadowMapViewMip0);
            graph.bindImage(kCascadeShadowMapBlurIntermediate1, mIntermediateShadowMapViewMip1);
            graph.bindImage(kCascadeShadowMapBlurIntermediate2, mIntermediateShadowMapViewMip2);

            graph.bindImage(kCascadeShadowMapBlured0, mBluredShadowMapViewMip0);
            graph.bindImage(kCascadeShadowMapBlured1, mBluredShadowMapViewMip1);
            graph.bindImage(kCascadeShadowMapBlured2, mBluredShadowMapViewMip2);
        }

        graph.bindBuffer(kCascadesInfo, *mCascadesBuffer);
    }

private:
    Shader mBlurXShader;

    Shader mBlurYShader;

    Shader mResolveShader;

    TaskID mRenderCascade0;
    TaskID mRenderCascade1;
    TaskID mRenderCascade2;

    Image mCascadeShadowMaps;
    ImageView mCascadeShaowMapsViewMip0;
    ImageView mCascadeShaowMapsViewMip1;
    ImageView mCascadeShaowMapsViewMip2;

    Image mResolvedShadowMap;
    ImageView mResolvedShadowMapView;

    Image mIntermediateShadowMap;
    ImageView mIntermediateShadowMapViewMip0;
    ImageView mIntermediateShadowMapViewMip1;
    ImageView mIntermediateShadowMapViewMip2;

    Image mBluredShadowMap;
    ImageView mBluredShadowMapViewMip0;
    ImageView mBluredShadowMapViewMip1;
    ImageView mBluredShadowMapViewMip2;

    PerFrameResource<Buffer> mCascadesBuffer;
    PerFrameResource<BufferView> mCascadesBufferView;
};

#endif
