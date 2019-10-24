#ifndef BLINNPHONETECHNIQUE
#define BLINNPHONGTECHNIQUE

#include "Core/Image.hpp"
#include "Technique.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include <string>

class Engine;

class BlinnPhongDeferredTexturesTechnique : public Technique
{
public:
    BlinnPhongDeferredTexturesTechnique(Engine* dev);
    virtual ~BlinnPhongDeferredTexturesTechnique() = default;

	Image& getLightingImage()
	{
		return mLightingTexture;
	}

	ImageView& getLightingView()
	{
		return mLightingView;
	}

    std::string getLightTextureName()
	{
        return kBlinnPhongLighting;
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

	virtual void bindResources(RenderGraph& graph) const override final
	{ graph.bindImage(kBlinnPhongLighting, mLightingView); }
	virtual void render(RenderGraph &, const std::vector<const Scene::MeshInstance *> &) override final;

private:

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
