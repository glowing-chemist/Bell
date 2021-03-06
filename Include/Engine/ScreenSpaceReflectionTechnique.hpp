#ifndef SCREEN_SAPCE_REFLECTION_TECHNIQUE_HPP
#define SCREEN_SAPCE_REFLECTION_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "Core/PerFrameResource.hpp"

class RenderEngine;
class RenderGraph;

class ScreenSpaceReflectionTechnique : public Technique
{
public:

	ScreenSpaceReflectionTechnique(RenderEngine*, RenderGraph&);
	~ScreenSpaceReflectionTechnique() = default;

	virtual PassType getPassType() const override final
	{
		return PassType::SSR;
	}

	virtual void render(RenderGraph&, RenderEngine*) override final {}

	virtual void bindResources(RenderGraph& graph) override final;

private:
    uint2 mTileCount;

    Image mReflectionMapRaw;
    ImageView mReflectionMapRawView;

    Image mReflectionMap;
    ImageView mReflectionMapView;

    Sampler mClampedSampler;
};


class RayTracedReflectionTechnique : public Technique
{
public:

    RayTracedReflectionTechnique(RenderEngine*, RenderGraph&);
    ~RayTracedReflectionTechnique() = default;

    virtual PassType getPassType() const override final
    {
        return PassType::RayTracedReflections;
    }

    virtual void render(RenderGraph&, RenderEngine*) override final {}

    virtual void bindResources(RenderGraph& graph) override final;

private:
    Image mReflectionMap;
    ImageView mReflectionMapView;

    uint32_t mSampleNumber;

    TaskID mTaskID;
};

#endif
