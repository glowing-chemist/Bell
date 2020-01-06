#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "UniformBuffers.glsl"

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 normals;
layout(location = 3) in uint material;

layout(location = 0) out vec4 positionVS;


layout(set = 0, binding = 0) uniform UniformBufferObject 
{    
    ShadowingLight light;
};


void main()
{
	gl_Position = light.viewProj * position;
	positionVS = light.view * position;
}