#ifndef GBUFFERMATERIAL_HPP
#define GBUFFERMATERIAL_HPP

#include "Technique.hpp"

#include "Core/Image.hpp"
#include "Core/ImageView.hpp"

#include "RenderGraph/GraphicsTask.hpp"

#include "Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


class GBufferMaterialTechnique : public Technique<GraphicsTask>
{
public:
	GBufferMaterialTechnique(Engine*);
	virtual ~GBufferMaterialTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::GBufferMaterial; }

    virtual GraphicsTask& getTask() final override
    { return mTask; }

	Image& getDepthImage()
		{ return mDepthImage; }

	ImageView& getDepthView()
		{ return mDepthView; }

	std::string getDepthName() const
        { return kGBufferDepth; }


	Image& getMaterialImage()
		{ return mMaterialImage; }

	ImageView& getMaterialView()
		{ return mMaterialView; }

    std::string getMaterialName() const
        { return kGBufferMaterialID; }


	Image& getNormalsImage()
		{ return mNormalsImage; }

	ImageView& getNormalsView()
		{ return mNormalsView; }

	std::string getNormalsName() const
     { return kGBufferNormals; }


	Image& getUVImage()
		{ return mUVImage; }

	ImageView& getUVView()
		{ return mUVView; }

	std::string getUVName() const
        { return kGBufferUV; }

    virtual void bindResources(RenderGraph&) const override final;

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
};


class GBufferMaterialPreDepthTechnique : public Technique<GraphicsTask>
{
public:
    GBufferMaterialPreDepthTechnique(Engine*);
    virtual ~GBufferMaterialPreDepthTechnique() = default;

    virtual PassType getPassType() const final override
        { return PassType::GBUfferMaterialPreDepth; }

    virtual GraphicsTask& getTask() final override
    { return mTask; }

    void setDepthName(const std::string& depthName)
        { mDepthName = depthName; }

    Image& getMaterialImage()
        { return mMaterialImage; }

    ImageView& getMaterialView()
        { return mMaterialView; }

    std::string getMaterialName() const
        { return kGBufferMaterialID; }


    Image& getNormalsImage()
        { return mNormalsImage; }

    ImageView& getNormalsView()
        { return mNormalsView; }

    std::string getNormalsName() const
     { return kGBufferNormals; }


    Image& getUVImage()
        { return mUVImage; }

    ImageView& getUVView()
        { return mUVView; }

    std::string getUVName() const
        { return kGBufferUV; }

    virtual void bindResources(RenderGraph&) const override final;

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
};

#endif
