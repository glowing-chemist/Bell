#ifndef OVERLAYTECHNIQUE_HPP
#define OVERLAYTECHNIQUE_HPP

#include "Core/RenderDevice.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"
#include "Core/PerFrameResource.hpp"

#include "Engine/DefaultResourceSlots.hpp"
#include "Technique.hpp"

class Engine;

class OverlayTechnique : public Technique
{
public:

    OverlayTechnique(Engine* dev, RenderGraph&);
    virtual ~OverlayTechnique() = default;

    virtual PassType getPassType() const override
    { return PassType::Overlay; }

    virtual void render(RenderGraph&, Engine*) override;

    virtual void bindResources(RenderGraph& graph) override;

private:
    Image mFontTexture;
    ImageView mFontImageView;

    PerFrameResource<Buffer> mOverlayUniformBuffer;
    PerFrameResource<BufferView> mOverlayerBufferView;

    PerFrameResource<Buffer> mOverlayVertexBuffer;
    PerFrameResource<BufferView> mOverlayerVertexBufferView;

    PerFrameResource<Buffer> mOverlayIndexBuffer;
    PerFrameResource<BufferView> mOverlayerIndexBufferView;

    GraphicsPipelineDescription mPipelineDescription;

    Shader mOverlayVertexShader;
    Shader mOverlayFragmentShader;

    TaskID mTaskID;
};

#endif
