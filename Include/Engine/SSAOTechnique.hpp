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


class SSAOTechnique : public Technique
{
public:
	SSAOTechnique(Engine* dev);
	virtual ~SSAOTechnique() = default;

	virtual PassType getPassType() const final override
		{ return PassType::SSAO; }

	virtual void bindResources(RenderGraph& graph) const override final 
	{
		graph.bindBuffer(kSSAOBuffer, *mSSAOBufferView);
	}

	virtual void render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&) override final;

private:

	std::string mDepthNameSlot;

	GraphicsPipelineDescription mPipelineDesc;
	GraphicsTask mTask;

	PerFrameResource<Buffer> mSSAOBuffer;
	PerFrameResource<BufferView> mSSAOBufferView;
};


class SSAOImprovedTechnique : public Technique
{
public:
	SSAOImprovedTechnique(Engine* dev);
	virtual ~SSAOImprovedTechnique() = default;

	virtual PassType getPassType() const final override
	{
		return PassType::SSAOImproved;
	}

	virtual void bindResources(RenderGraph& graph) const override final 
	{
		graph.bindBuffer(kSSAOBuffer, *mSSAOBufferView);
	}

	virtual void render(RenderGraph& graph, Engine*, const std::vector<const Scene::MeshInstance*>&) override final;

private:

	std::string mDepthNameSlot;

	GraphicsPipelineDescription mPipelineDesc;
	GraphicsTask mTask;

	PerFrameResource<Buffer> mSSAOBuffer;
	PerFrameResource<BufferView> mSSAOBufferView;
};


#endif
