#ifndef SKYBOX_TECHNIQUE_HPP
#define SKYBOX_TECHNIQUE_HPP

#include "Technique.hpp"

#include "RenderGraph/GraphicsTask.hpp"


class Engine;


class SkyboxTechnique : public Technique
{
public:
    SkyboxTechnique(Engine*);
    virtual ~SkyboxTechnique() = default;

    virtual PassType getPassType() const override final
    { return PassType::Skybox; }

    // default empty implementations as most classes won't need to do anything for one of these.
    virtual void render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&) override final
    { graph.addTask(mTask); }

    virtual void bindResources(RenderGraph&) const override final
    {}

private:
    GraphicsPipelineDescription mPipelineDesc;
    GraphicsTask mTask;
};

#endif
