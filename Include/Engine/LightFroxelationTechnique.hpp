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
	LightFroxelationTechnique(Engine*);
	~LightFroxelationTechnique() = default;

    virtual PassType getPassType() const override final
	{
		return PassType::LightFroxelation;
	}

    virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance*>&) override final;

    virtual void bindResources(RenderGraph&) override final;

private:

    ComputePipelineDescription      mActiveFroxelsDesc;
    ComputeTask                     mActiveFroxels;

    ComputePipelineDescription      mIndirectArgsDesc;
    ComputeTask                     mIndirectArgs;

    ComputePipelineDescription      mClearCountersDesc;
    ComputeTask                     mClearCounters;

    ComputePipelineDescription      mLightAsignmentDesc;
    ComputeTask                     mLightListAsignment;

    PerFrameResource<Image>         mActiveFroxelsImage;
    PerFrameResource<ImageView>     mActiveFroxelsImageView;

    PerFrameResource<Buffer>        mActiveFroxlesBuffer;
    PerFrameResource<BufferView>    mActiveFroxlesBufferView;
    PerFrameResource<BufferView>	mActiveFroxelsCounter;

    PerFrameResource<Buffer>		mIndirectArgsBuffer;
    PerFrameResource<BufferView>	mIndirectArgsView;

    PerFrameResource<Buffer>        mSparseFroxelBuffer;
    PerFrameResource<BufferView>    mSparseFroxelBufferView;

    PerFrameResource<Buffer>        mLightIndexBuffer;
    PerFrameResource<BufferView>    mLightIndexBufferView;
    PerFrameResource<BufferView>    mLightIndexCounterView;
};

#endif
