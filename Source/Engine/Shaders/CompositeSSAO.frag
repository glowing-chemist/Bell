#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in float2 uv;

layout(location = 0) out float4 frameBuffer;


layout(binding = 0) uniform texture2D globalLighting;
layout(binding = 1) uniform texture2D ssao;
layout(binding = 2) uniform sampler defaultSampler;

void main()
{
	const float4 lighting = texture(sampler2D(globalLighting, defaultSampler), uv);
	const float4 aoGather = textureGather(sampler2D(ssao, defaultSampler), uv);
	const float ao = dot(aoGather, float4(1.0f)) / 4.0f;

	frameBuffer = lighting * ao;
}