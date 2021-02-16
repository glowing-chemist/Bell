#ifndef SSAOTECHNIQUE_HPP
#define SSAOTECHNIQUE_HPP

#include "Core/Image.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"
#include "Core/PerFrameResource.hpp"
#include "Engine/Technique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include <string>

extern const char kSSAORaw[];
extern const char kSSAOHistory[];
extern const char kSSAOCounter[];
extern const char kSSAOSampler[];
extern const char kSSAOBlurX[];
extern const char kSSAOBlurY[];

class SSAOTechnique : public Technique
{
public:
	SSAOTechnique(RenderEngine* dev, RenderGraph&);
	virtual ~SSAOTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::SSAO; }

    virtual void bindResources(RenderGraph& graph) override final
	{
		graph.bindBuffer(kSSAOBuffer, *mSSAOBufferView);

        const size_t index = getDevice()->getCurrentSubmissionIndex() % 2;
        graph.bindImage(kSSAORaw, mSSAOViews[index]);
        graph.bindImage(kSSAOHistory, mSSAOViews[1 - index]);

        graph.bindImage(kSSAOCounter, mHistoryCounterViews);

        if(!graph.isResourceSlotBound(kSSAO))
        {
            graph.bindImage(kSSAO, mSSAOViews[2]);
            graph.bindImage(kSSAOBlurX, mSSAOBlurView[0]);
            graph.bindImage(kSSAOBlurY, mSSAOBlurView[1]);
            graph.bindSampler(kSSAOSampler, mNearestSampler);
        }
	}

	virtual void render(RenderGraph& graph, RenderEngine*) override final;

private:

	std::string mDepthNameSlot;

	GraphicsPipelineDescription mPipelineDesc;

    Shader mFulllscreenTriangleShader;
    Shader mSSAOShader;

    bool mFirstFrame;

    Image mSSAO[3];
    ImageView mSSAOViews[3];

    Image mSSAOBlur[2];
    ImageView mSSAOBlurView[2];

    Image mHistoryCounter;
    ImageView mHistoryCounterViews;

    Sampler mNearestSampler;

	PerFrameResource<Buffer> mSSAOBuffer;
	PerFrameResource<BufferView> mSSAOBufferView;
};



#endif
