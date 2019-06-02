#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec2 uv;


layout(location = 0) out float SSAOOutput;


layout(binding = 0) uniform UniformBuffer1
{    
    vec4 offsets[16];
    int numberOfOffsets; 
} normalsOffsets;

layout(binding = 1) uniform UniformBuffer2
{    
    mat4 model;    
    mat4 view;    
    mat4 perspective;    
} camera; 

layout(binding = 2) uniform texture2D depthTexture;

layout(binding = 3) uniform sampler linearSampler;


void main()
{
	const float depth = texture(sampler2D(depthTexture, linearSampler), uv).x;

	float occlusion = 1.0f;
	const float occlusionFactor = 1.0f / float(normalsOffsets.numberOfOffsets);

	for(uint i = 0; i < normalsOffsets.numberOfOffsets; ++i)
	{
		const vec4 scaledViewSpaceNormalOffsets = (camera.perspective * camera.view * (normalsOffsets.offsets[i] / 100.0f));

		const vec4 offsetedPosition = vec4(uv, depth, 1.0f) + scaledViewSpaceNormalOffsets;

		const float realOffsetDepth = texture(sampler2D(depthTexture, linearSampler), offsetedPosition.xy).x;

		occlusion -= occlusionFactor * mix(0.0f, 1.0f, (offsetedPosition.z - realOffsetDepth) > 0.0f);
	}

	SSAOOutput = occlusion;
}
