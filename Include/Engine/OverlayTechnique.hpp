#ifndef OVERLAYTECHNIQUE_HPP
#define OVERLAYTECHNIQUE_HPP

#include "Core/RenderDevice.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"

#include "Engine/DefaultResourceSlots.hpp"
#include "Technique.hpp"

class Engine;

class OverlayTechnique : public Technique
{
public:

    OverlayTechnique(Engine* dev);
    virtual ~OverlayTechnique() = default;

    virtual PassType getPassType() const override
    { return PassType::Overlay; }

    virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance *>&) override;

    virtual void bindResources(RenderGraph& graph) const override
    {
		graph.bindImage(kDefaultFontTexture, mFontImageView);
		graph.bindBuffer("OverlayUBO", mOverlayerBufferView);
		graph.bindSampler(kDefaultSampler, mFontSampler);
		// TODO remove temp
		graph.bindImage(kFrameBufer, getDevice()->getSwapChainImageView());
    }

private:
    Image mFontTexture;
    ImageView mFontImageView;

    Buffer mOverlayUniformBuffer;
    BufferView mOverlayerBufferView;

    Sampler mFontSampler;

    GraphicsPipelineDescription mPipelineDescription;
};

#endif
