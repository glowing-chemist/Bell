#ifndef VOXALIZE_TECHNIQUE_HPP
#define VOXALIZE_TECHNIQUE_HPP

#include "Engine/Technique.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Image.hpp"
#include "Core/ImageView.hpp"
#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"


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
        graph.bindBuffer("VoxelDimm", mVoxelDimmensionsView);
    }

private:

    struct ConstantBuffer
    {
        float4 voxelCentreWS;
        float4 recipVoxelSize; // mapes from world -> to voxel Coords;
        float4 recipVolumeSize; // maps from voxel coords to clip space.
    };

    PerFrameResource<Image> mVoxelMap;
    PerFrameResource<ImageView> mVoxelMapView;

    Buffer mVoxelDimmensions;
    BufferView mVoxelDimmensionsView;

    GraphicsPipelineDescription mPipelineDesc;
    TaskID mTaskID;
};

#endif
