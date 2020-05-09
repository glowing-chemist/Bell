#ifndef CASCADE_SHADOW_MAPPING_TECHNIQUE_HPP
#define CASCADE_SHADOW_MAPPING_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "Engine/DefaultResourceSlots.hpp"

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
        graph.bindImage(kCascadeShadowMapRaw0, *mCascadeShaowMapsViewMip0);
        graph.bindImage(kCascadeShadowMapRaw1, *mCascadeShaowMapsViewMip1);
        graph.bindImage(kCascadeShadowMapRaw2, *mCascadeShaowMapsViewMip2);

        graph.bindImage(kShadowMap, *mResolvedShadowMapView);

        graph.bindImage(kCascadeShadowMapBlurIntermediate0, *mIntermediateShadowMapViewMip0);
        graph.bindImage(kCascadeShadowMapBlurIntermediate1, *mIntermediateShadowMapViewMip1);
        graph.bindImage(kCascadeShadowMapBlurIntermediate2, *mIntermediateShadowMapViewMip2);

        graph.bindImage(kCascadeShadowMapBlured0, *mBluredShadowMapViewMip0);
        graph.bindImage(kCascadeShadowMapBlured1, *mBluredShadowMapViewMip1);
        graph.bindImage(kCascadeShadowMapBlured2, *mBluredShadowMapViewMip2);

        graph.bindBuffer(kCascadesInfo, *mCascadesBuffer);
    }

private:
    ComputePipelineDescription mBlurXDesc;

    ComputePipelineDescription mBlurYDesc;

    ComputePipelineDescription mResolveDesc;

    TaskID mRenderCascade0;
    TaskID mRenderCascade1;
    TaskID mRenderCascade2;

    PerFrameResource<Image> mCascadeShadowMaps;
    PerFrameResource<ImageView> mCascadeShaowMapsViewMip0;
    PerFrameResource<ImageView> mCascadeShaowMapsViewMip1;
    PerFrameResource<ImageView> mCascadeShaowMapsViewMip2;

    PerFrameResource<Image> mResolvedShadowMap;
    PerFrameResource<ImageView> mResolvedShadowMapView;

    PerFrameResource<Image> mIntermediateShadowMap;
    PerFrameResource<ImageView> mIntermediateShadowMapViewMip0;
    PerFrameResource<ImageView> mIntermediateShadowMapViewMip1;
    PerFrameResource<ImageView> mIntermediateShadowMapViewMip2;

    PerFrameResource<Image> mBluredShadowMap;
    PerFrameResource<ImageView> mBluredShadowMapViewMip0;
    PerFrameResource<ImageView> mBluredShadowMapViewMip1;
    PerFrameResource<ImageView> mBluredShadowMapViewMip2;

    PerFrameResource<Buffer> mCascadesBuffer;
    PerFrameResource<BufferView> mCascadesBufferView;
};

#endif
