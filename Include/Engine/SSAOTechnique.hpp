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

constexpr char kSSAORough[] = "SSAORough";
constexpr char kSSAOBlurIntermidiate[] = "SSAOBlurInt";


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

        // request the needed resources.
        graph.createTransientImage(getDevice(), kSSAO, Format::R8UNorm, ImageUsage::Sampled | ImageUsage::Storage, SizeClass::HalfSwapchain);
        graph.createTransientImage(getDevice(), kSSAOBlurIntermidiate, Format::R8UNorm, ImageUsage::Sampled | ImageUsage::Storage, SizeClass::HalfSwapchain);

	}

	virtual void render(RenderGraph& graph, Engine*) override final;

private:

	std::string mDepthNameSlot;

	GraphicsPipelineDescription mPipelineDesc;

    ComputePipelineDescription mBlurXDesc;

    ComputePipelineDescription mBlurYDesc;

	PerFrameResource<Buffer> mSSAOBuffer;
	PerFrameResource<BufferView> mSSAOBufferView;
};


class SSAOImprovedTechnique : public Technique
{
public:
	SSAOImprovedTechnique(Engine* dev, RenderGraph&);
	virtual ~SSAOImprovedTechnique() = default;

	virtual PassType getPassType() const final override
	{
		return PassType::SSAOImproved;
	}

    virtual void bindResources(RenderGraph& graph) override final
	{
		graph.bindBuffer(kSSAOBuffer, *mSSAOBufferView);

        // request the needed resources.
        graph.createTransientImage(getDevice(), kSSAO, Format::R8UNorm, ImageUsage::Sampled | ImageUsage::Storage, SizeClass::Swapchain);
        graph.createTransientImage(getDevice(), kSSAOBlurIntermidiate, Format::R8UNorm, ImageUsage::Sampled | ImageUsage::Storage, SizeClass::Swapchain);
	}

	virtual void render(RenderGraph& graph, Engine*) override final;

private:

	std::string mDepthNameSlot;

	GraphicsPipelineDescription mPipelineDesc;

    ComputePipelineDescription mBlurXDesc;

    ComputePipelineDescription mBlurYDesc;

	PerFrameResource<Buffer> mSSAOBuffer;
	PerFrameResource<BufferView> mSSAOBufferView;
};


#endif
