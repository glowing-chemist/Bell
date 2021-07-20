#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "Core/DeviceChild.hpp"
#include "Core/Shader.hpp"
#include "Core/RenderDevice.hpp"
#include "RenderGraph/RenderTask.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/ComputeTask.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "Engine/GeomUtils.h"

#include <vulkan/vulkan.hpp>

#include <optional>


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

	void setDebugName(const std::string& name)
	{
	    mDebugName= name;
	}

	const std::string& getDebugName() const
	{
	    return mDebugName;
	}

protected:
	std::string mDebugName;
	vk::Pipeline mPipeline;
	vk::PipelineLayout mPipelineLayout;
};


class PipelineTemplate : public DeviceChild
{
public:
    PipelineTemplate(RenderDevice*, const GraphicsPipelineDescription*, const vk::PipelineLayout layout);

    std::shared_ptr<Pipeline> instanciateGraphicsPipeline(const GraphicsTask&,
                                                          const uint64_t hashPrefix,
                                                          const vk::RenderPass rp,
                                                          const int vertexAttributes,
                                                          const Shader& vertexShader,
                                                          const Shader* geometryShader,
                                                          const Shader* tessControl,
                                                          const Shader* tessEval,
                                                          const Shader& fragmentShader);

    std::shared_ptr<Pipeline> instanciateComputePipeline(const ComputeTask &task, const uint64_t hashPrefix, const Shader& computeShader);

    void invalidatePipelineCache();

    vk::PipelineLayout getLayoutHandle() const
    {
        return mPipelineLayout;
    }

private:
    bool mGraphicsPipeline;
    GraphicsPipelineDescription mDesc;
    vk::PipelineLayout mPipelineLayout;

    std::unordered_map<uint64_t, std::shared_ptr<Pipeline>> mPipelineCache;

};


class ComputePipeline : public Pipeline
{
public:
    ComputePipeline(RenderDevice* dev, const Shader& computeShader) :
        Pipeline{dev},
        mComputeShader(computeShader) {}

	 virtual bool compile(const RenderTask&) override final;


private:

    Shader mComputeShader;

};


class GraphicsPipeline : public Pipeline
{
public:

    GraphicsPipeline(RenderDevice* dev, const GraphicsPipelineDescription& desc,
                     const Shader& vertexShader,
                     const Shader* geometryShader,
                     const Shader* tessControl,
                     const Shader* tessEval,
                     const Shader& fragmentShader) :
		Pipeline{dev},
        mPipelineDescription{ desc },
        mVertexShader(vertexShader),
        mGeometryShader(geometryShader ? *geometryShader : std::optional<Shader>{}),
        mTessControlShader(tessControl ? *tessControl : std::optional<Shader>{}),
        mTessEvalShader(tessEval ? *tessEval : std::optional<Shader>{}),
        mFragmentShader(fragmentShader) {}

    std::vector<vk::PipelineShaderStageCreateInfo> generateShaderStagesInfo();

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

private:

	GraphicsPipelineDescription mPipelineDescription;
    Shader mVertexShader;
    std::optional<Shader> mGeometryShader;
    std::optional<Shader> mTessControlShader;
    std::optional<Shader> mTessEvalShader;
    Shader mFragmentShader;

	std::vector< vk::DescriptorSetLayout> mDescSetLayout;
	vk::VertexInputBindingDescription mVertexDescription;
	std::vector<vk::VertexInputAttributeDescription> mVertexAttribs;
	vk::RenderPass mRenderPass;
    std::vector<vk::PipelineColorBlendAttachmentState> mBlendStates;
};

#endif
