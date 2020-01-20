#ifndef IMAGEBASEDLIGHTING_TECHNIQUE_HPP
#define IMAGEBASEDLIGHTING_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"


class ImageBasedLightingDeferredTexturingTechnique : public Technique
{
public:

    ImageBasedLightingDeferredTexturingTechnique(Engine*);
    ~ImageBasedLightingDeferredTexturingTechnique() = default;

	virtual PassType getPassType() const override final
	{
		return PassType::DeferredTexturePBRIBL;
	}

	virtual void render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&) override final
	{
		graph.addTask(mTask);
	}

    virtual void bindResources(RenderGraph&) override final {}

private:

	GraphicsPipelineDescription mPipelineDesc;
	GraphicsTask mTask;
};


class DeferredImageBasedLightingTechnique : public Technique
{
public:

    DeferredImageBasedLightingTechnique(Engine*);
    ~DeferredImageBasedLightingTechnique() = default;

    virtual PassType getPassType() const override final
    {
        return PassType::DeferredPBRIBL;
    }

    virtual void render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&) override final
    {
        graph.addTask(mTask);
    }

    virtual void bindResources(RenderGraph&) override final {}

private:

    GraphicsPipelineDescription mPipelineDesc;
    GraphicsTask mTask;
};


#endif
