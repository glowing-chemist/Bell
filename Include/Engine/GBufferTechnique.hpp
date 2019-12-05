#ifndef GBUFFER_HPP
#define GBUFFER_HPP

#include "Technique.hpp"

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"

#include "RenderGraph/GraphicsTask.hpp"

#include "Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


class GBufferTechnique : public Technique
{
public:
	GBufferTechnique(Engine*);
	virtual ~GBufferTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::GBuffer; }

	virtual void bindResources(RenderGraph&) const override final {};
	virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance *> &) override final;

private:

	GraphicsPipelineDescription mPipelineDescription;
	GraphicsTask mTask;
};


class GBufferPreDepthTechnique : public Technique
{
public:
    GBufferPreDepthTechnique(Engine*);
    virtual ~GBufferPreDepthTechnique() = default;

    virtual PassType getPassType() const final override
        { return PassType::GBufferPreDepth; }


	virtual void bindResources(RenderGraph&) const override final {}
	virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance *> &) override final;

private:

    GraphicsPipelineDescription mPipelineDescription;
    GraphicsTask mTask;
};



#endif
