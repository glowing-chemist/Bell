#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 uv;


layout(location = 0) out vec4 frameBuffer;


layout(binding = 1) uniform textureCube skyBox;
layout(binding = 2) uniform sampler defaultSampler;

void main()
{
	frameBuffer = texture(samplerCube(skyBox, defaultSampler), uv);
}