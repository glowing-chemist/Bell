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

    virtual void bindResources(RenderGraph& graph) override final
	{}
	virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance *> &) override final;

private:

	std::string mDepthName;
	std::string mVertexNormalsName;
	std::string mMaterialName;
	std::string mUVName;

	GraphicsPipelineDescription mPipelineDesc;
	GraphicsTask mTask;
};

#endif
