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
        float4 minimum;
        float4 maximum;
        uint3  offset;
        float  voxelSize;
        uint32_t lod;
    };

    struct TerrainTexturing
    {
        float2 textureScale;
        uint materialIndexXZ;
        uint materialIndexY;
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
