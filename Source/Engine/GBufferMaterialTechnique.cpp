#include "Engine/GBufferMaterialTechnique.hpp"


GBufferMaterialTechnique::GBufferMaterialTechnique(Engine* eng) :
    Technique{"GBufferMaterial", eng->getDevice()},

	mDepthImage{getDevice(), Format::D24S8Float, ImageUsage::DepthStencil | ImageUsage::Sampled,
				getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				1, 1, 1, 1, "Depth Buffer"},
	mDepthView{mDepthImage, ImageViewType::Depth},

	mMaterialImage{getDevice(), Format::R32Uint, ImageUsage::ColourAttachment | ImageUsage::Sampled,
				 getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				 1, 1, 1, 1, "Material Buffer"},
	mMaterialView{mMaterialImage, ImageViewType::Colour},

	mNormalsImage{getDevice(), Format::R16G16Unorm, ImageUsage::ColourAttachment | ImageUsage::Sampled,
				  getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				  1, 1, 1, 1, "Normals Buffer"},
	mNormalsView{mNormalsImage, ImageViewType::Colour},

	mUVImage{getDevice(), Format::RGBA32SFloat, ImageUsage::ColourAttachment | ImageUsage::Sampled,
				   getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
				   1, 1, 1, 1, "UV Buffer"},
	mUVView{mUVImage, ImageViewType::Colour},

    mPipelineDescription{eng->getShader("./Shaders/GBufferPassThroughMaterial.vert"),
                         eng->getShader("./Shaders/GBufferMaterial.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual},

	mTask{"GBufferMaterial", mPipelineDescription},
	mTaskInitialised{false}
{}


GraphicsTask &GBufferMaterialTechnique::getTaskToRecord()
{

	if(!mTaskInitialised)
	{
		mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
								  VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

		mTask.addInput("CameraBuffer", AttachmentType::UniformBuffer);
		mTask.addInput("ModelMatrix", AttachmentType::PushConstants);

        mTask.addOutput("GBuffer Normals",  AttachmentType::Texture2D, Format::R16G16Unorm, LoadOp::Clear_Black);
        mTask.addOutput("GBuffer UV",       AttachmentType::Texture2D, Format::RGBA8SRGB, LoadOp::Clear_Black);
		mTask.addOutput("GBuffer Material", AttachmentType::Texture2D, Format::R8UNorm, LoadOp::Clear_Black);
        mTask.addOutput("GBuffer Depth",    AttachmentType::Depth, Format::D24S8Float, LoadOp::Clear_White);

		mTaskInitialised = true;
	}

	mTask.clearCalls();

	return mTask;
}


GBufferMaterialPreDepthTechnique::GBufferMaterialPreDepthTechnique(Engine* eng) :
    Technique{"GBufferMaterial", eng->getDevice()},

    mMaterialImage{getDevice(), Format::R32Uint, ImageUsage::ColourAttachment | ImageUsage::Sampled,
                 getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
                 1, 1, 1, 1, "Material Buffer"},
    mMaterialView{mMaterialImage, ImageViewType::Colour},

    mNormalsImage{getDevice(), Format::R16G16Unorm, ImageUsage::ColourAttachment | ImageUsage::Sampled,
                  getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
                  1, 1, 1, 1, "Normals Buffer"},
    mNormalsView{mNormalsImage, ImageViewType::Colour},

    mUVImage{getDevice(), Format::RGBA32SFloat, ImageUsage::ColourAttachment | ImageUsage::Sampled,
                   getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
                   1, 1, 1, 1, "UV Buffer"},
    mUVView{mUVImage, ImageViewType::Colour},

    mPipelineDescription{eng->getShader("./Shaders/GBufferPassThroughMaterial.vert"),
                         eng->getShader("./Shaders/GBufferMaterial.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual},

    mTask{"GBufferMaterialPreDepth", mPipelineDescription},
    mTaskInitialised{false}
{}


GraphicsTask &GBufferMaterialPreDepthTechnique::getTaskToRecord()
{

    if(!mTaskInitialised)
    {
        mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Material |
                                  VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

        mTask.addInput("CameraBuffer", AttachmentType::UniformBuffer);
        mTask.addInput("ModelMatrix", AttachmentType::PushConstants);

        mTask.addOutput("GBuffer Normals",  AttachmentType::Texture2D, Format::R16G16Unorm, LoadOp::Clear_Black);
        mTask.addOutput("GBuffer UV",       AttachmentType::Texture2D, Format::RGBA8SRGB, LoadOp::Clear_Black);
        mTask.addOutput("GBuffer Material", AttachmentType::Texture2D, Format::R8UNorm, LoadOp::Clear_Black);
        mTask.addOutput(mDepthName,         AttachmentType::Depth, Format::D24S8Float, LoadOp::Preserve);

        mTaskInitialised = true;
    }

    mTask.clearCalls();

    return mTask;
}
