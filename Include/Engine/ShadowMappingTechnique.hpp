#ifndef SHADOW_MAPPING_HPP
#define SHADOW_MAPPING_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"


class ShadowMappingTechnique : public Technique
{
public:
    ShadowMappingTechnique(Engine*);
    ~ShadowMappingTechnique();

    virtual PassType getPassType() const override
    {
        return PassType::Shadow;
    }

    virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance*>&) override;

    virtual void bindResources(RenderGraph&) const override {}

private:
    GraphicsPipelineDescription mDesc;
    GraphicsTask                mTask;
};

#endif
