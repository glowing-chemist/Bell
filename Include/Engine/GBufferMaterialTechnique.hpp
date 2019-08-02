#ifndef GBUFFERMATERIAL_HPP
#define GBUFFERMATERIAL_HPP

#include "Technique.hpp"

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"

#include "RenderGraph/GraphicsTask.hpp"

#include "Engine.hpp"


class GBufferMaterialTechnique : public Technique<GraphicsTask>
{
public:
	GBufferMaterialTechnique(Engine*);
	virtual ~GBufferMaterialTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::GBufferMaterial; }

	virtual GraphicsTask& getTaskToRecord() final override;

	Image& getDepthImage()
		{ return mDepthImage; }

	ImageView& getDepthView()
		{ return mDepthView; }

	std::string getDepthName() const
		{ return "GBuffer Depth"; }


	Image& getMaterialImage()
		{ return mMaterialImage; }

	ImageView& getMaterialView()
		{ return mMaterialView; }

	std::string getAlbedoName() const
		{ return "GBuffer Material"; }


	Image& getNormalsImage()
		{ return mNormalsImage; }

	ImageView& getNormalsView()
		{ return mNormalsView; }

	std::string getNormalsName() const
	 { return "GBuffer Normals"; }


	Image& getUVImage()
		{ return mUVImage; }

	ImageView& getUVView()
		{ return mUVView; }

	std::string getUVName() const
		{ return "GBuffer UV"; }

private:

	Image mDepthImage;
	ImageView mDepthView;

	Image mMaterialImage;
	ImageView mMaterialView;

	Image mNormalsImage;
	ImageView mNormalsView;

	Image mUVImage;
	ImageView mUVView;

	GraphicsPipelineDescription mPipelineDescription;
	GraphicsTask mTask;
	bool mTaskInitialised;
};


class GBufferMaterialPreDepthTechnique : public Technique<GraphicsTask>
{
public:
    GBufferMaterialPreDepthTechnique(Engine*);
    virtual ~GBufferMaterialPreDepthTechnique() = default;

    virtual PassType getPassType() const final override
        { return PassType::GBUfferMaterialPreDepth; }

    virtual GraphicsTask& getTaskToRecord() final override;

    void setDepthName(const std::string& depthName)
        { mDepthName = depthName; }

    Image& getMaterialImage()
        { return mMaterialImage; }

    ImageView& getMaterialView()
        { return mMaterialView; }

    std::string getAlbedoName() const
        { return "GBuffer Material"; }


    Image& getNormalsImage()
        { return mNormalsImage; }

    ImageView& getNormalsView()
        { return mNormalsView; }

    std::string getNormalsName() const
     { return "GBuffer Normals"; }


    Image& getUVImage()
        { return mUVImage; }

    ImageView& getUVView()
        { return mUVView; }

    std::string getUVName() const
        { return "GBuffer UV"; }

private:

    std::string mDepthName;

    Image mMaterialImage;
    ImageView mMaterialView;

    Image mNormalsImage;
    ImageView mNormalsView;

    Image mUVImage;
    ImageView mUVView;

    GraphicsPipelineDescription mPipelineDescription;
    GraphicsTask mTask;
    bool mTaskInitialised;
};

#endif
