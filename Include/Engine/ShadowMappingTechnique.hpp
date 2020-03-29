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

    virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance*>&) override;

    virtual void bindResources(RenderGraph& graph) override 
    {
        graph.createTransientImage(getDevice(), kShadowMap, Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled, SizeClass::Swapchain);
        graph.createTransientImage(getDevice(), kShadowMapBlurIntermediate, Format::RG32Float, ImageUsage::Storage | ImageUsage::Sampled, SizeClass::DoubleSwapchain);
        graph.createTransientImage(getDevice(), kShadowMapBlured, Format::RG32Float, ImageUsage::Storage | ImageUsage::Sampled, SizeClass::DoubleSwapchain);
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
};

#endif
