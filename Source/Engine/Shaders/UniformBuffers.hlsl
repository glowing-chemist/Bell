

struct CameraBuffer    
{     
        float4x4 view;    
        float4x4 perspective;
        float4x4 viewProj;
            
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
};    

    
struct SSAOBuffer    
{    
        float4 offsets[64];
        float scale;
        int offsetsCount;      
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

struct MaterialAttributes
{
    uint materialAttributes;
};