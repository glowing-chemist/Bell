#ifndef VOXALIZE_TECHNIQUE_HPP
#define VOXALIZE_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"

#define DEBUG_VOXEL_GENERATION 1

class VoxalizeTechnique : public Technique
{
public:
    VoxalizeTechnique(Engine*, RenderGraph&);
    ~VoxalizeTechnique() = default;

    virtual PassType getPassType() const override final
    {
        return PassType::Voxelize;
    }

    virtual void render(RenderGraph&, Engine*) override final;

    virtual void bindResources(RenderGraph& graph) override final
    {
        graph.bindImage(kDiffuseVoxelMap, *mVoxelMapView);
        graph.bindBuffer(kVoxelDimmensions, *mVoxelDimmensionsView);

#if DEBUG_VOXEL_GENERATION
        graph.bindImage(kDebugVoxels, *mVoxelDebugView);
#endif
    }

private:

    struct ConstantBuffer
    {
        float4 voxelCentreWS;
        float4 recipVoxelSize; // mapes from world -> to voxel Coords;
        float4 recipVolumeSize; // maps from voxel coords to clip space.
        float4 voxelVolumeSize;
    };

    PerFrameResource<Image> mVoxelMap;
    PerFrameResource<ImageView> mVoxelMapView;

    PerFrameResource<Buffer> mVoxelDimmensions;
    PerFrameResource<BufferView> mVoxelDimmensionsView;

    GraphicsPipelineDescription mPipelineDesc;
    TaskID mTaskID;

#if DEBUG_VOXEL_GENERATION
    PerFrameResource<Image> mVoxelDebug;
    PerFrameResource<ImageView> mVoxelDebugView;

    ComputePipelineDescription mDebugVoxelPipeline;
#endif
};

#endif
