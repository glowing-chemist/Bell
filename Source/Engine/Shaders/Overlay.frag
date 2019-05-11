#version 450            
#extension GL_ARB_separate_shader_objects : enable    
    
    
layout(location = 0) in vec2 inText;         
layout(location = 1) in vec4 inAlbedo;

layout(location = 0) out vec4 UITexture;

layout(binding = 1) uniform texture2D fontTexture;
layout(binding = 2) uniform sampler linearSampler;

    
void main()
{
	vec4 fonts = texture(sampler2D(fontTexture, linearSampler), inText);

	if(fonts.w == 0.0f)
		discard;

	UITexture = inAlbedo * fonts;
}
