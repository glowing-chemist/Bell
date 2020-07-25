#ifndef OCCLUSION_CULLING_TECHNIQUE_HPP
#define OCCLUSION_CULLING_TECHNIQUE_HPP

#include "Technique.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"
#include "Core/PerFrameResource.hpp"


class OcclusionCullingTechnique : public Technique
{
public:
    OcclusionCullingTechnique(Engine*, RenderGraph&);
    ~OcclusionCullingTechnique() = default;

    virtual PassType getPassType() const
    {
        return PassType::OcclusionCulling;
    }

    virtual void render(RenderGraph&, Engine*) {};

    virtual void bindResources(RenderGraph&);

private:

    ComputePipelineDescription mPipelineDesc;

    PerFrameResource<Buffer> mBoundsIndexBuffer;
    PerFrameResource<BufferView> mBoundsIndexBufferView;

    Buffer mPredicationBuffer;
    BufferView mPredicationBufferView;

    Sampler mOcclusionSampler;
};

#endif
