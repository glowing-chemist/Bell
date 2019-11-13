#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 frameBuffer;


layout(binding = 0) uniform texture2D globalLighting;
layout(binding = 1) uniform texture2D overlay;
layout(binding = 2) uniform sampler defaultSampler;

void main()
{
	const vec4 lighting = texture(sampler2D(globalLighting, defaultSampler), uv);
	const vec4 overlay = texture(sampler2D(overlay, defaultSampler), uv);

	frameBuffer = ((1.0f - overlay.w) * lighting) + overlay;;
}
