#include "Engine/SkyboxTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

SkyboxTechnique::SkyboxTechnique(Engine* eng, RenderGraph& graph) :
	Technique("Skybox", eng->getDevice()),
	mPipelineDesc{eng->getShader("./Shaders/SkyBox.vert"),
						 eng->getShader("./Shaders/SkyBox.frag"),
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
							   getDevice()->getSwapChain()->getSwapChainImageHeight()},
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
						 getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         false, BlendMode::None, BlendMode::None, false, DepthTest::Equal, Primitive::TriangleList}
{
	GraphicsTask task{ "skybox", mPipelineDesc };
	task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	task.addInput(kSkyBox, AttachmentType::Texture2D);
	task.addInput(kDefaultSampler, AttachmentType::Sampler);

	task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm);
	task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);

	task.setRecordCommandsCallback(
		[](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
		{
			exec->draw(0, 3);
		}
	);
	mTaskID = graph.addTask(task);
}
