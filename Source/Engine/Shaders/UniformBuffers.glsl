

struct CameraBuffer    
{     
        mat4 view;    
        mat4 perspective;    
            
        mat4 invertedCamera;      
      
        vec2 frameBufferSize;

        vec3 position;
};    
    
    
struct SSAOBuffer    
{    
        vec4 offsets[16];
        float scale;
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
