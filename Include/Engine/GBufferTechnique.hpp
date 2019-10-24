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

	std::string getDepthName() const
        { return kGBufferDepth; }

	std::string getAlbedoName() const
        { return kGBufferAlbedo; }


	std::string getNormalsName() const
     { return kGBufferNormals; }


	std::string getSpecularName() const
        { return kGBufferSpecular; }

	virtual void bindResources(RenderGraph&) const override final {};
	virtual void render(RenderGraph &, const std::vector<const Scene::MeshInstance *> &) override final;

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

    void setDepthName(const std::string& depthName)
    {  mDepthName = depthName; }

	std::string getAlbedoName() const
        { return kGBufferAlbedo; }


	std::string getNormalsName() const
     { return kGBufferNormals; }


	std::string getSpecularName() const
        { return kGBufferSpecular; }

	virtual void bindResources(RenderGraph&) const override final {}
	virtual void render(RenderGraph &, const std::vector<const Scene::MeshInstance *> &) override final;

private:

    std::string mDepthName;

    GraphicsPipelineDescription mPipelineDescription;
    GraphicsTask mTask;
};



#endif
