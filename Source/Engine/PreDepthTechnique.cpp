#include "Engine/PreDepthTechnique.hpp"


PreDepthTechnique::PreDepthTechnique(Engine* eng) :
    Technique{"PreDepth", eng->getDevice()},

    mDepthImage{getDevice(), Format::D24S8Float, ImageUsage::DepthStencil | ImageUsage::Sampled,
                getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
                1, 1, 1, 1, "Depth Buffer"},
    mDepthView{mDepthImage, ImageViewType::Depth},

    mPipelineDescription{eng->getShader("./Shaders/GBufferPassThrough.vert"),
                         eng->getShader("./Shaders/Empty.frag"),
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                               getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                         getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         true, BlendMode::None, BlendMode::None, true, DepthTest::GreaterEqual, Primitive::TriangleList},

    mTask{"PreDepth", mPipelineDescription}
{
    mTask.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates);

    mTask.addOutput(getDepthName(), AttachmentType::Depth, Format::D32Float, LoadOp::Clear_White);
}
