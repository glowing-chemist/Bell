#ifndef IMAGEBASEDLIGHTING_TECHNIQUE_HPP
#define IMAGEBASEDLIGHTING_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/GraphicsTask.hpp"


class ImageBasedLightingDeferredTexturingTechnique : public Technique
{
public:

    ImageBasedLightingDeferredTexturingTechnique(Engine*, RenderGraph&);
    ~ImageBasedLightingDeferredTexturingTechnique() = default;

	virtual PassType getPassType() const override final
	{
		return PassType::DeferredTexturePBRIBL;
	}

	virtual void render(RenderGraph& graph, Engine*) override final
	{}

    virtual void bindResources(RenderGraph&) override final {}

private:

	GraphicsPipelineDescription mPipelineDesc;
    TaskID mTaskID;
};


class DeferredImageBasedLightingTechnique : public Technique
{
public:

    DeferredImageBasedLightingTechnique(Engine*, RenderGraph&);
    ~DeferredImageBasedLightingTechnique() = default;

    virtual PassType getPassType() const override final
    {
        return PassType::DeferredPBRIBL;
    }

    virtual void render(RenderGraph& graph, Engine*) override final
    {}

    virtual void bindResources(RenderGraph&) override final {}

private:

    GraphicsPipelineDescription mPipelineDesc;
    TaskID mTaskID;
};


#endif
