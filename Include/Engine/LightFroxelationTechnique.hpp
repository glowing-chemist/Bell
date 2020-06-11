#ifndef LIGHT_FROXELATION_TECHNIQUE_HPP
#define LIGHT_FROXELATION_TECHNIQUE_HPP

#include "Technique.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "RenderGraph/ComputeTask.hpp"
#include "Core/PerFrameResource.hpp"


class LightFroxelationTechnique : public Technique
{
public:
	LightFroxelationTechnique(Engine*, RenderGraph&);
	~LightFroxelationTechnique() = default;

    virtual PassType getPassType() const override final
	{
		return PassType::LightFroxelation;
	}

    virtual void render(RenderGraph&, Engine*) override final;

    virtual void bindResources(RenderGraph&) override final;

private:

    ComputePipelineDescription      mActiveFroxelsDesc;
    TaskID                          mActiveFroxels;

    ComputePipelineDescription      mIndirectArgsDesc;
    TaskID                          mIndirectArgs;

    ComputePipelineDescription      mClearCountersDesc;
    TaskID                          mClearCounters;

    ComputePipelineDescription      mLightAsignmentDesc;
    TaskID                          mLightListAsignment;

    uint32_t mXTiles;
    uint32_t mYTiles;

    Image         mActiveFroxelsImage;
    ImageView     mActiveFroxelsImageView;

    Buffer        mActiveFroxlesBuffer;
    BufferView    mActiveFroxlesBufferView;
    BufferView	mActiveFroxelsCounter;

    Buffer		mIndirectArgsBuffer;
    BufferView	mIndirectArgsView;

    Buffer        mSparseFroxelBuffer;
    BufferView    mSparseFroxelBufferView;

    Buffer        mLightIndexBuffer;
    BufferView    mLightIndexBufferView;
    BufferView    mLightIndexCounterView;
};

#endif
