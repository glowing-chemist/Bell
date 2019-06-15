#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable


#include "UniformBuffers.glsl"


layout(location = 0) in vec2 uv;


layout(location = 0) out float SSAOOutput;


layout(binding = 0) uniform UniformBuffer1
{    
    SSAOBuffer normalsOffsets;
};

layout(binding = 1) uniform UniformBuffer2
{        
    CameraBuffer camera;   
}; 

layout(binding = 2) uniform texture2D depthTexture;

layout(binding = 3) uniform sampler linearSampler;


void main()
{
	const float depth = texture(sampler2D(depthTexture, linearSampler), uv).x;

	// early out if the depth for this pixel is at the far clip plane.
	if(depth == 1.0f)
	{
		SSAOOutput = 1.0f;
		return;
	}

	float occlusion = 1.0f;
	const float occlusionFactor = 1.0f / float(normalsOffsets.offsetsCount);

	for(uint i = 0; i < normalsOffsets.offsetsCount; ++i)
	{
		const vec4 scaledViewSpaceNormalOffsets = (camera.perspective * camera.view * (normalsOffsets.offsets[i] * normalsOffsets.scale));

		const vec4 offsetedPosition = vec4(uv, depth, 1.0f) + scaledViewSpaceNormalOffsets;

		const float realOffsetDepth = texture(sampler2D(depthTexture, linearSampler), offsetedPosition.xy).x;

		occlusion -= occlusionFactor * mix(0.0f, 1.0f, (offsetedPosition.z - realOffsetDepth) > 0.0f);
	}

	SSAOOutput = occlusion;
}
