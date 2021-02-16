#include "Engine/SkyboxTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

SkyboxTechnique::SkyboxTechnique(RenderEngine* eng, RenderGraph& graph) :
	Technique("Skybox", eng->getDevice()),
    mPipelineDesc{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                  getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                  getDevice()->getSwapChain()->getSwapChainImageHeight()},
                  FaceWindingOrder::None, BlendMode::None, BlendMode::None, false, DepthTest::Equal, FillMode::Fill, Primitive::TriangleList},
    mSkyboxVertexShader(eng->getShader("./Shaders/SkyBox.vert")),
    mSkyboxFragmentShader(eng->getShader("./Shaders/SkyBox.frag"))
{
	GraphicsTask task{ "skybox", mPipelineDesc };
	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kSkyBox, AttachmentType::CubeMap);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);

    task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA16Float);
	task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);

	task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine*, const std::vector<const MeshInstance*>&)
		{
            PROFILER_EVENT("render skybox");
            PROFILER_GPU_TASK(exec);
            PROFILER_GPU_EVENT("render skybox");

            const RenderTask& task = graph.getTask(taskIndex);
            exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mSkyboxVertexShader, nullptr, nullptr, nullptr, mSkyboxFragmentShader);

			exec->draw(0, 3);
		}
	);
	mTaskID = graph.addTask(task);
}
