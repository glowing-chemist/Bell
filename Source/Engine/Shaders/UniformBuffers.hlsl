

struct CameraBuffer    
{     
        float4x4 view;    
        float4x4 perspective;
        float4x4 viewProj;
            
        float4x4 invertedView;
        float4x4 invertedViewProj;
        float4x4 invertedPerspective;

        float4x4 previousFrameViewProj;

        float nearPlane;
        float farPlane;
      
        float2 frameBufferSize;

        float3 position;
        float fov;

        float2 jitter;
        float2 previousJitter;

        float4 sceneSize;
};    

    
struct SSAOBuffer    
{    
        float projScale;
        float radius;
        float bias;
        float intensity;
        float4 projInfo;   
};


struct ShadowingLight
{
        float4x4 view;
        float4x4 inverseView;
        float4x4 viewProj;
        float4 position;
        float4 direction;
        float4 up;
};


struct ShadowCascades
{
    float near;
    float mid;
    float far;
};


struct VoxelDimmensions
{
    float4 voxelCentreWS;
    float4 recipVoxelSize; // mapes from world -> to voxel Coords;
    float4 recipVolumeSize; // maps from voxel coords to clip space.
    float4 voxelVolumeSize;
};