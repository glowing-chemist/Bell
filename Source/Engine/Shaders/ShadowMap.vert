#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "UniformBuffers.glsl"

layout(location = 0) in float4 position;
layout(location = 1) in float2 uv;
layout(location = 2) in float4 normals;
layout(location = 3) in uint material;

layout(location = 0) out float4 positionVS;
layout(location = 1) out float2 outUV;
layout(location = 2) out uint outMaterial;


layout(set = 0, binding = 0) uniform UniformBufferObject 
{    
    ShadowingLight light;
};

layout(push_constant) uniform pushModelMatrix
{
	float4x4 mesh;
	float4x4 previousMesh;
} push_constants;


void main()
{
	gl_Position = light.viewProj * push_constants.mesh * position;
	positionVS = light.view * push_constants.mesh * position;
	outUV = uv;
	outMaterial = material;
}