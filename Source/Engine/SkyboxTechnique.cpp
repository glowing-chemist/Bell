#include "Engine/SkyboxTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

SkyboxTechnique::SkyboxTechnique(Engine* eng) :
	Technique("Skybox", eng->getDevice()),
	mPipelineDesc{eng->getShader("./Shaders/SkyBox.vert"),
						 eng->getShader("./Shaders/SkyBox.frag"),
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
							   getDevice()->getSwapChain()->getSwapChainImageHeight()},
						 Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
						 getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         false, BlendMode::None, BlendMode::None, false, DepthTest::Equal, Primitive::TriangleList},
	mTask{"skybox", mPipelineDesc}
{
	mTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
	mTask.addInput(kSkyBox, AttachmentType::Texture2D);
	mTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    mTask.addOutput(kFrameBufer, AttachmentType::RenderTarget2D, getDevice()->getSwapChainImage().getFormat());
    mTask.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);

	mTask.addDrawCall(0, 3);
}
