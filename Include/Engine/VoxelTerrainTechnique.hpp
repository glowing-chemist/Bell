#ifndef VOXEL_TERRAIN_TECHNIQUE_HPP
#define VOXEL_TERRAIN_TECHNIQUE_HPP

#include "Technique.hpp"
#include "Core/PerFrameResource.hpp"

class VoxelTerrainTechnique : public Technique
{
public:

    VoxelTerrainTechnique(RenderEngine*, RenderGraph&);

    virtual PassType getPassType() const final override
    { return PassType::VoxelTerrain; }

    virtual void bindResources(RenderGraph&) override final;
    virtual void render(RenderGraph&, RenderEngine*) override final
    {
        mVoxelGrid->updateLastAccessed();
        mVoxelGridView->updateLastAccessed();
        mVertexBuffer->updateLastAccessed();
        mIndirectArgsBuffer->updateLastAccessed();
    }

    void setTextureScale(const float2& scale)
    {
        mTextureScale = scale;
    }

    void setMaterialIndexXZ(const uint32_t index)
    {
        mMaterialIndexXZ = index;
    }

    void setMaterialIndexY(const uint32_t index)
    {
        mMaterialIndexY = index;
    }

private:

    struct TerrainVolume
    {
        float3 minimum;
        float  voxelSize;
        uint3  offset;
        uint32_t   lod;
        float4 frustumPlanes[6];
    };

    struct TerrainTexturing
    {
        float2 textureScale;
        uint32_t materialIndexXZ;
        uint32_t materialIndexY;
    };

    struct TerrainModifying
    {
        uint2 mMousePos;
        float mValue;
        float mSize;
        float3 mTerrainSize;
    };

    Shader mGenerateTerrainMeshShader;
    Shader mInitialiseIndirectDrawShader;

    Shader mTerrainVertexShader;
    Shader mTerrainFragmentShaderDeferred;

    Shader mModifyTerrainShader;

    Image mVoxelGrid;
    ImageView mVoxelGridView;

    Buffer mVertexBuffer;
    BufferView mVertexBufferView;

    Buffer mIndirectArgsBuffer;
    BufferView mIndirectArgView;

    TaskID mSurfaceGenerationTask;
    TaskID mRenderTaskID;

    float mModifySize;
    float2 mTextureScale;
    uint32_t mMaterialIndexXZ;
    uint32_t mMaterialIndexY;
};

#endif
