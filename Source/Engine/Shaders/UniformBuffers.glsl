

struct CameraBuffer    
{     
        mat4 view;    
        mat4 perspective;
        mat4 viewProj;
            
        mat4 invertedViewProj;
        mat4 invertedPerspective;

        mat4 previousFrameViewProj;

        float nearPlane;
        float farPlane;
      
        vec2 frameBufferSize;

        vec3 position;
        float fov;
};    

    
struct SSAOBuffer    
{    
        vec4 offsets[64];
        float scale;
        int offsetsCount;      
};


struct ShadowingLight
{
        mat4 view;
        mat4 inverseView;
        mat4 viewProj;
        vec4 position;
        vec4 direction;
};

