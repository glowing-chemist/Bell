#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in float2 uv;

layout(location = 0) out float4 frameBuffer;


layout(binding = 0) uniform texture2D globalLighting;
layout(binding = 1) uniform texture2D analyticalLighting;
layout(binding = 2) uniform texture2D ssao;
layout(binding = 3) uniform texture2D overlay;
layout(binding = 4) uniform sampler defaultSampler;


void main()
{
	const float4 globalLight = texture(sampler2D(globalLighting, defaultSampler), uv);
	const float4 analyticalLight = texture(sampler2D(analyticalLighting, defaultSampler), uv);
	const float4 lighting = globalLight + analyticalLight;
	const float4 aoGather = textureGather(sampler2D(ssao, defaultSampler), uv);
	const float ao = dot(aoGather, float4(1.0f)) / 4.0f;
	const float4 overlay = texture(sampler2D(overlay, defaultSampler), uv);

	frameBuffer = ((1.0f - overlay.w) * (lighting * ao)) + overlay;
}
