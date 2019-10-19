#ifndef GBUFFER_HPP
#define GBUFFER_HPP

#include "Technique.hpp"

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"

#include "RenderGraph/GraphicsTask.hpp"

#include "Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


class GBufferTechnique : public Technique<GraphicsTask>
{
public:
	GBufferTechnique(Engine*);
	virtual ~GBufferTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::GBuffer; }

    virtual GraphicsTask& getTask() final override
    { return mTask; }

	std::string getDepthName() const
        { return kGBufferDepth; }

	std::string getAlbedoName() const
        { return kGBufferAlbedo; }


	std::string getNormalsName() const
     { return kGBufferNormals; }


	std::string getSpecularName() const
        { return kGBufferSpecular; }

    virtual void bindResources(RenderGraph&) const override final;

private:

	GraphicsPipelineDescription mPipelineDescription;
	GraphicsTask mTask;
};


class GBufferPreDepthTechnique : public Technique<GraphicsTask>
{
public:
    GBufferPreDepthTechnique(Engine*);
    virtual ~GBufferPreDepthTechnique() = default;

    virtual PassType getPassType() const final override
        { return PassType::GBufferPreDepth; }

    void setDepthName(const std::string& depthName)
    {  mDepthName = depthName; }

    virtual GraphicsTask& getTask() final override
    { return mTask; }

        std::string getAlbedoName() const
        { return kGBufferAlbedo; }


        std::string getNormalsName() const
     { return kGBufferNormals; }


        std::string getSpecularName() const
        { return kGBufferSpecular; }

    virtual void bindResources(RenderGraph&) const override final;

private:

    std::string mDepthName;

    GraphicsPipelineDescription mPipelineDescription;
    GraphicsTask mTask;
};



#endif
