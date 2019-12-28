#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "Core/DeviceChild.hpp"
#include "Core/Shader.hpp"
#include "Core/RenderDevice.hpp"
#include "RenderGraph/RenderTask.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "Engine/GeomUtils.h"

#include <vulkan/vulkan.hpp>

#include <optional>


class RenderDevice;

class Pipeline : public DeviceChild
{
public:

	Pipeline(RenderDevice*);
	virtual ~Pipeline() = default;

	virtual bool compile(const RenderTask&) = 0;

	vk::Pipeline getHandle() const
	{
		return mPipeline;
	}

	vk::PipelineLayout getLayoutHandle() const
	{
		return mPipelineLayout;
	}

	void setLayout(const vk::PipelineLayout layout)
	{
		mPipelineLayout = layout;
	}

protected:

	vk::Pipeline mPipeline;
	vk::PipelineLayout mPipelineLayout;
};


class ComputePipeline : public Pipeline
{
public:
	ComputePipeline(RenderDevice* dev, const ComputePipelineDescription& desc) :
		Pipeline{dev},
		mPipelineDescription{ desc } {}

	 virtual bool compile(const RenderTask&) override final;


private:

	ComputePipelineDescription mPipelineDescription;

};


class GraphicsPipeline : public Pipeline
{
public:

	GraphicsPipeline(RenderDevice* dev, const GraphicsPipelineDescription& desc) :
		Pipeline{dev},
		mPipelineDescription{ desc } {}

	virtual bool compile(const RenderTask&) override final;

	void setVertexAttributes(const int);
	void setFrameBufferBlendStates(const RenderTask&);
	void setRenderPass(const vk::RenderPass pass)
	{
		mRenderPass = pass;
	}
	void setDescriptorLayouts(const std::vector< vk::DescriptorSetLayout>& layout)
	{
		mDescSetLayout = layout;
	}

	vk::Pipeline getInstancedVariant() const
	{
		return mInstancedVariant;
	}

private:

	GraphicsPipelineDescription mPipelineDescription;

	std::vector< vk::DescriptorSetLayout> mDescSetLayout;
	vk::VertexInputBindingDescription mVertexDescription;
	std::vector<vk::VertexInputAttributeDescription> mVertexAttribs;
	vk::RenderPass mRenderPass;
	std::vector<vk::PipelineColorBlendAttachmentState> mBlendStates;



	vk::Pipeline mInstancedVariant;
};

#endif
