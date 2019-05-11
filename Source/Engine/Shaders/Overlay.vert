#version 450            
#extension GL_ARB_separate_shader_objects : enable    
    
    
layout(location = 0) in vec2 fragPos;    
layout(location = 1) in vec2 inText;     
layout(location = 2) in vec4 inAlbedo;


layout(binding = 0) uniform UniformBufferObject 
{    
    vec4 trans;    
} ubo;

layout(location = 0) out vec2 outTexCoord;    
layout(location = 1) out vec4 outAlbedo;

out gl_PerVertex {    
    vec4 gl_Position;    
};    
      
    
void main()
{
        gl_Position = vec4(fragPos * ubo.trans.xy + ubo.trans.zw, 0.0f, 1.0f);
        outTexCoord = inText;
	outAlbedo = inAlbedo;
}
