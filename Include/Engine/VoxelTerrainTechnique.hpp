#ifndef VOXEL_TERRAIN_TECHNIQUE_HPP
#define VOXEL_TERRAIN_TECHNIQUE_HPP

#include "Technique.hpp"
#include "Core/PerFrameResource.hpp"

class VoxelTerrainTechnique : public Technique
{
public:

    VoxelTerrainTechnique(Engine*, RenderGraph&);

    virtual PassType getPassType() const final override
    { return PassType::VoxelTerrain; }

    virtual void bindResources(RenderGraph&) override final;
    virtual void render(RenderGraph&, Engine*) override final
    {
	mVoxelGrid->updateLastAccessed();
	mVoxelGridView->updateLastAccessed();
    mVertexBuffer->updateLastAccessed();
    mIndirectArgsBuffer->updateLastAccessed();
    }

private:

    struct TerrainVolume
    {
        float4 minimum;
        float4 maximum;
        uint3  offset;
        float  voxelSize;
    };

    Shader mGenerateTerrainMeshShader;
    Shader mInitialiseIndirectDrawShader;

    Shader mTerrainVertexShader;
    Shader mTerrainFragmentShaderDeferred;

    Image mVoxelGrid;
    ImageView mVoxelGridView;

    Buffer mVertexBuffer;
    BufferView mVertexBufferView;

    Buffer mIndirectArgsBuffer;
    BufferView mIndirectArgView;

    TaskID mSurfaceGenerationTask;
    TaskID mRenderTaskID;
};

#endif
