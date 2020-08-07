#ifndef GBUFFERMATERIAL_HPP
#define GBUFFERMATERIAL_HPP

#include "Technique.hpp"

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"

#include "RenderGraph/GraphicsTask.hpp"

#include "Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


class GBufferMaterialTechnique : public Technique
{
public:
	GBufferMaterialTechnique(Engine*, RenderGraph&);
	virtual ~GBufferMaterialTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::GBufferMaterial; }

	std::string getDepthName() const
        { return kGBufferDepth; }


	std::string getMaterialName() const
        { return kGBufferMaterialID; }


	std::string getNormalsName() const
     { return kGBufferNormals; }


	std::string getUVName() const
        { return kGBufferUV; }

    virtual void bindResources(RenderGraph&) override final {}
    virtual void render(RenderGraph&, Engine*) override final {}

private:
    
	GraphicsPipelineDescription mPipelineDescription;

    Shader mGbufferVertexShader;
    Shader mGbufferFragmentShader;

    TaskID mTaskID;
};


class GBufferMaterialPreDepthTechnique : public Technique
{
public:
    GBufferMaterialPreDepthTechnique(Engine*, RenderGraph&);
    virtual ~GBufferMaterialPreDepthTechnique() = default;

    virtual PassType getPassType() const final override
        { return PassType::GBUfferMaterialPreDepth; }


    void setDepthName(const std::string& depthName)
        { mDepthName = depthName; }


    std::string getMaterialName() const
        { return kGBufferMaterialID; }


    std::string getNormalsName() const
     { return kGBufferNormals; }


    std::string getUVName() const
        { return kGBufferUV; }

    virtual void bindResources(RenderGraph&) override final {}
    virtual void render(RenderGraph&, Engine*) override final {}

private:

    std::string mDepthName;

    GraphicsPipelineDescription mPipelineDescription;

    Shader mGbufferVertexShader;
    Shader mGbufferFragmentShader;

    TaskID mTaskID;
};

#endif
