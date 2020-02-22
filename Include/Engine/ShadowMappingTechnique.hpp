#ifndef SHADOW_MAPPING_HPP
#define SHADOW_MAPPING_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "Engine/DefaultResourceSlots.hpp"


class ShadowMappingTechnique : public Technique
{
public:
    ShadowMappingTechnique(Engine*);
    ~ShadowMappingTechnique() = default;

    virtual PassType getPassType() const override
    {
        return PassType::Shadow;
    }

    virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance*>&) override;

    virtual void bindResources(RenderGraph& graph) override 
    {
        graph.createTransientImage(getDevice(), kShadowMap, Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled, SizeClass::Swapchain);
    }

private:
    GraphicsPipelineDescription mDesc;
    GraphicsTask                mTask;

    ComputePipelineDescription mResolveDesc;
    ComputeTask                mResolveTask;
};

#endif
