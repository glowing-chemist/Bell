#include "Engine/DeferredAnalyticalLightingTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Core/Executor.hpp"

DeferredAnalyticalLightingTechnique::DeferredAnalyticalLightingTechnique(RenderEngine* eng, RenderGraph& graph) :
	Technique("deferred analytical lighting", eng->getDevice()),
    mDeferredAnalitucalLightingShader( eng->getShader("./Shaders/DeferredAnalyticalLighting.comp") ),
    mAnalyticalLighting(eng->getDevice(), Format::RGBA16Float, ImageUsage::Sampled | ImageUsage::Storage, eng->getSwapChainImageView()->getImageExtent().width,
		eng->getSwapChainImageView()->getImageExtent().height, 1, 1, 1, 1, kAnalyticLighting),
    mAnalyticalLightingView(mAnalyticalLighting, ImageViewType::Colour)
{
    ComputeTask task{ "deferred analytical lighting" };
	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	task.addInput(kDFGLUT, AttachmentType::Texture2D);
	task.addInput(kLTCMat, AttachmentType::Texture2D);
	task.addInput(kLTCAmp, AttachmentType::Texture2D);
	task.addInput(kGBufferDepth, AttachmentType::Texture2D);
	task.addInput(kGBufferNormals, AttachmentType::Texture2D);
    task.addInput(kGBufferDiffuse, AttachmentType::Texture2D);
    task.addInput(kGBufferSpecularRoughness, AttachmentType::Texture2D);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);
	task.addInput(kPointSampler, AttachmentType::Sampler);
	task.addInput(kSparseFroxels, AttachmentType::DataBufferRO);
	task.addInput(kLightIndicies, AttachmentType::DataBufferRO);
	task.addInput(kActiveFroxels, AttachmentType::Texture2D);
	task.addInput(kAnalyticLighting, AttachmentType::Image2D);
	task.addInput(kLightBuffer, AttachmentType::ShaderResourceSet);

	task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>&)
		{
			PROFILER_EVENT("deferred lighting");
			PROFILER_GPU_TASK(exec);
			PROFILER_GPU_EVENT("deferred lighting");

            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mDeferredAnalitucalLightingShader);

			const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
			const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

			exec->dispatch(	static_cast<uint32_t>(std::ceil(threadGroupWidth / 32.0f)),
							static_cast<uint32_t>(std::ceil(threadGroupHeight / 32.0f)),
							1);
		}
	);

	mTaskID = graph.addTask(task);
}


void DeferredAnalyticalLightingTechnique::bindResources(RenderGraph& graph)
{
    if(!graph.isResourceSlotBound(kAnalyticLighting))
        graph.bindImage(kAnalyticLighting, mAnalyticalLightingView);
}
