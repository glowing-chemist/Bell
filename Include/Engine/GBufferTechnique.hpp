#ifndef GBUFFER_HPP
#define GBUFFER_HPP

#include "Technique.hpp"

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"

#include "RenderGraph/GraphicsTask.hpp"

#include "Engine.hpp"


class GBufferTechnique : public Technique<GraphicsTask>
{
public:
	GBufferTechnique(Engine*);
	virtual ~GBufferTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::GBuffer; }

	virtual GraphicsTask& getTaskToRecord() final override;

	Image& getDepthImage()
		{ return mDepthImage; }

	ImageView& getDepthView()
		{ return mDepthView; }

	std::string getDepthName() const
		{ return "GBuffer Depth"; }


	Image& getAlbedoImage()
		{ return mAlbedoImage; }

	ImageView& getAlbedoView()
		{ return mAlbedoView; }

	std::string getAlbedoName() const
		{ return "GBuffer Albedo"; }


	Image& getNormalsImage()
		{ return mNormalsImage; }

	ImageView& getNormalsView()
		{ return mNormalsView; }

	std::string getNormalsName() const
	 { return "GBuffer Normals"; }


	Image& getSpecularImage()
		{ return mSpecularImage; }

	ImageView& getSpecularView()
		{ return mSpecularView; }

	std::string getSpecularName() const
		{ return "Gbuffer Specular"; }

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
	bool mTaskInitialised;
};


class GBufferPreDepthTechnique : public Technique<GraphicsTask>
{
public:
    GBufferPreDepthTechnique(Engine*);
    virtual ~GBufferPreDepthTechnique() = default;

    virtual PassType getPassType() const final override
        { return PassType::GBufferPreDepth; }

    virtual GraphicsTask& getTaskToRecord() final override;

    void setDepthName(const std::string& depthName)
    {  mDepthName = depthName; }


    Image& getAlbedoImage()
        { return mAlbedoImage; }

    ImageView& getAlbedoView()
        { return mAlbedoView; }

    std::string getAlbedoName() const
        { return "GBuffer Albedo"; }


    Image& getNormalsImage()
        { return mNormalsImage; }

    ImageView& getNormalsView()
        { return mNormalsView; }

    std::string getNormalsName() const
     { return "GBuffer Normals"; }


    Image& getSpecularImage()
        { return mSpecularImage; }

    ImageView& getSpecularView()
        { return mSpecularView; }

    std::string getSpecularName() const
        { return "Gbuffer Specular"; }

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
    bool mTaskInitialised;
};



#endif
