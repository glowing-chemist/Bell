#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 frameBuffer;


layout(binding = 0) uniform texture2D tex;
layout(binding = 1) uniform sampler defaultSampler;


void main()
{
	const vec4 pixel = texture(sampler2D(tex, defaultSampler), uv);

	frameBuffer = pixel;
}
