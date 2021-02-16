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
	GBufferTechnique(RenderEngine*, RenderGraph&);
	virtual ~GBufferTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::GBuffer; }

    virtual void bindResources(RenderGraph&) override final {}
    virtual void render(RenderGraph&, RenderEngine*) override final {}

private:

	GraphicsPipelineDescription mPipelineDescription;

	TaskID mTaskID;
};


class GBufferPreDepthTechnique : public Technique
{
public:
    GBufferPreDepthTechnique(RenderEngine*, RenderGraph&);
    virtual ~GBufferPreDepthTechnique() = default;

    virtual PassType getPassType() const final override
        { return PassType::GBufferPreDepth; }


    virtual void bindResources(RenderGraph&) override final {}
    virtual void render(RenderGraph&, RenderEngine*) override final {}

private:

    GraphicsPipelineDescription mPipelineDescription;

	TaskID mTaskID;
};



#endif
