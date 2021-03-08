#ifndef INSTANCE_ID_TECHNIQUE_HPP
#define INSTANCE_ID_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "Core/PerFrameResource.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"

class InstanceIDTechnique : public Technique
{
public:

    InstanceIDTechnique(RenderEngine* eng, RenderGraph& graph);

    virtual PassType getPassType() const override final
    {
        return PassType::InstanceID;
    }

    virtual void render(RenderGraph&, RenderEngine*) override final;

    virtual void bindResources(RenderGraph&) override final;

    void setMousePosition(const uint2& pos)
    {
        mCurrentMousePos = pos;
    }

    InstanceID getCurrentlySelectedInstanceID() const
    {
        return mSelectedInstanceID;
    }

private:
    TaskID mTaskID;
    GraphicsPipelineDescription mPipeline;
    Shader mVertexShader;
    Shader mFragmentShader;
    Shader mComputeShader;

    struct PushConstants
    {
        float4x4 transform;
        InstanceID id;
    };

    uint2 mCurrentMousePos;
    uint32_t mSelectedInstanceID;

    PerFrameResource<Buffer> mSelectedIDBuffer;
    PerFrameResource<BufferView> mSelectedIDBufferView;
};

#endif