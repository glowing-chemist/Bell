#ifndef BLINNPHONETECHNIQUE
#define BLINNPHONGTECHNIQUE

#include "Core/Image.hpp"
#include "Technique.hpp"

#include <string>

class Engine;

class BlinnPhongDeferredTextures : public Technique<GraphicsTask>
{
public:
	BlinnPhongDeferredTextures(Engine* dev);
	virtual ~BlinnPhongDeferredTextures() = default;

	Image& getLightingImage()
	{
		return mLightingTexture;
	}

	ImageView& getLightingView()
	{
		return mLightingView;
	}

	static std::string& getLightTextureName()
	{
		static std::string name{"BlinnPhongLightTexture"};

		return name;
	}

	void setDepthName(const std::string& name)
	{
		mDepthName = name;
	}

	void setVertexNormalName(const std::string& name)
	{
		mVertexNormalsName = name;
	}

	void setMaterialName(const std::string& name)
	{
		mMaterialName = name;
	}

	void setUVName(const std::string& name)
	{
		mUVName = name;
	}

	virtual PassType getPassType() const final override
	{
		return PassType::DeferredTextureBlinnPhongLighting;
	}

	virtual GraphicsTask& getTaskToRecord() override final;



private:

	bool mTaskInitialised;

	std::string mDepthName;
	std::string mVertexNormalsName;
	std::string mMaterialName;
	std::string mUVName;

	Image mLightingTexture;
	ImageView mLightingView;

	GraphicsPipelineDescription mPipelineDesc;
	GraphicsTask mTask;
};

#endif
