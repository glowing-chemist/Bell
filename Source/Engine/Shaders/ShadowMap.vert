#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "UniformBuffers.glsl"

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 normals;
layout(location = 3) in uint material;

layout(location = 0) out vec4 positionVS;
layout(location = 1) out vec2 outUV;
layout(location = 2) out uint outMaterial;


layout(set = 0, binding = 0) uniform UniformBufferObject 
{    
    ShadowingLight light;
};

layout(push_constant) uniform pushModelMatrix
{
	mat4 model;
} push_constants;


void main()
{
	gl_Position = light.viewProj * push_constants.model * position;
	positionVS = light.view * push_constants.model * position;
	outUV = uv;
	outMaterial = material;
}