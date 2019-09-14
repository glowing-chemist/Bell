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

	Image& getDepthImage()
		{ return mDepthImage; }

	ImageView& getDepthView()
		{ return mDepthView; }

	std::string getDepthName() const
        { return kGBufferDepth; }


	Image& getAlbedoImage()
		{ return mAlbedoImage; }

	ImageView& getAlbedoView()
		{ return mAlbedoView; }

	std::string getAlbedoName() const
        { return kGBufferAlbedo; }


	Image& getNormalsImage()
		{ return mNormalsImage; }

	ImageView& getNormalsView()
		{ return mNormalsView; }

	std::string getNormalsName() const
     { return kGBufferNormals; }


	Image& getSpecularImage()
		{ return mSpecularImage; }

	ImageView& getSpecularView()
		{ return mSpecularView; }

	std::string getSpecularName() const
        { return kGBufferSpecular; }

    virtual void bindResources(RenderGraph&) const override final;

private:

	Image mDepthImage;
	ImageView mDepthView;

	Image mAlbedoImage;
	ImageView mAlbedoView;

	Image mNormalsImage;
	ImageView mNormalsView;

	Image mSpecularImage;
	ImageView mSpecularView;

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

    Image& getAlbedoImage()
        { return mAlbedoImage; }

    ImageView& getAlbedoView()
        { return mAlbedoView; }

    std::string getAlbedoName() const
        { return kGBufferAlbedo; }


    Image& getNormalsImage()
        { return mNormalsImage; }

    ImageView& getNormalsView()
        { return mNormalsView; }

    std::string getNormalsName() const
     { return kGBufferNormals; }


    Image& getSpecularImage()
        { return mSpecularImage; }

    ImageView& getSpecularView()
        { return mSpecularView; }

    std::string getSpecularName() const
        { return kGBufferSpecular; }

    virtual void bindResources(RenderGraph&) const override final;

private:

    std::string mDepthName;

    Image mAlbedoImage;
    ImageView mAlbedoView;

    Image mNormalsImage;
    ImageView mNormalsView;

    Image mSpecularImage;
    ImageView mSpecularView;

    GraphicsPipelineDescription mPipelineDescription;
    GraphicsTask mTask;
};



#endif
