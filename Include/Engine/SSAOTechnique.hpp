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

class SSAOTechnique : public Technique
{
public:
	SSAOTechnique(Engine* dev, RenderGraph&);
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
            graph.bindImage(kSSAO, mSSAOViews[2]);
	}

	virtual void render(RenderGraph& graph, Engine*) override final;

private:

	std::string mDepthNameSlot;

	GraphicsPipelineDescription mPipelineDesc;

    Shader mFulllscreenTriangleShader;
    Shader mSSAOShader;

    bool mFirstFrame;

    Image mSSAO[3];
    ImageView mSSAOViews[3];

    Image mHistoryCounter;
    ImageView mHistoryCounterViews;

	PerFrameResource<Buffer> mSSAOBuffer;
	PerFrameResource<BufferView> mSSAOBufferView;
};



#endif
