#ifndef SKINNING_TECHNIQUE_HPP
#define SKINNING_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "RenderGraph/ComputeTask.hpp"


class SkinningTechnique : public Technique
{
public:
    SkinningTechnique(RenderEngine*, RenderGraph&);
    virtual ~SkinningTechnique() = default;

    virtual PassType getPassType() const final override
        { return PassType::Animation; }

    virtual void bindResources(RenderGraph&) override final;
    virtual void render(RenderGraph&, RenderEngine*) override final {}

    struct PushConstant
    {
        uint32_t mVertexCount;
        uint32_t mVertexReadIndex;
        uint32_t mVertexWriteIndex;
        uint32_t mBoneIndex;
        uint32_t mBlendShapeReadIndex;
        uint32_t mVertexStride;
    };

private:
    Shader mSkinningShader;
    Shader mSkinningBlendShapesShader;
    Buffer mBlendShapeScratchBuffer;
    BufferView mBlendShapeScratchBufferView;
    TaskID mTask;
};

#endif
