

struct CameraBuffer    
{     
        mat4 view;    
        mat4 perspective;    
            
        mat4 invertedCamera;
        mat4 invertedPerspective;

        float nearPlane;
        float farPlane;
      
        vec2 frameBufferSize;

        vec3 position;
        float fov;
};    
    
    
struct SSAOBuffer    
{    
        vec4 offsets[16];
        float scale;
        int offsetsCount;      
};

