

struct CameraBuffer    
{     
        mat4 view;    
        mat4 perspective;    
            
        mat4 invertedCamera;      
      
        vec4 frameBufferSize;    
};    
    
    
struct SSAOBuffer    
{    
        vec4 offsets[16];    
        int offsetsCount;      
};


struct Light
{
		vec4 position;
		vec4 albedo;
		float influence;
		float FOV;
		float brightness;
};
