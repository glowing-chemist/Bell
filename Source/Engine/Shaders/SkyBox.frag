#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "UniformBuffers.glsl"

layout(location = 0) in vec2 uv;


layout(location = 0) out vec4 frameBuffer;


layout(binding = 1) uniform textureCube skyBox;
layout(binding = 2) uniform sampler defaultSampler;

layout(binding = 0) uniform UniformBufferObject {    
    CameraBuffer camera;    
};

void main()
{
	vec4 worldSpaceFragmentPos = camera.invertedViewProj * vec4((uv - 0.5f) * 2.0f, 0.00001f, 1.0f);
    worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

    const vec3 cubemapUV = normalize(worldSpaceFragmentPos.xyz - camera.position);

    frameBuffer = texture(samplerCube(skyBox, defaultSampler), cubemapUV);
}