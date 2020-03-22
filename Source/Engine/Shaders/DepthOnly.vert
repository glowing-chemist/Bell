#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "UniformBuffers.glsl"

layout(location = 0) in float4 position;
layout(location = 1) in float2 uv;
layout(location = 2) in float4 normals;
layout(location = 3) in uint material;

layout(location = 0) out float2 outUV;
layout(location = 1) out uint outMatreialID;

out gl_PerVertex {
    float4 gl_Position;
};


layout(binding = 0) uniform UniformBufferObject {    
    CameraBuffer camera;    
}; 

layout(push_constant) uniform pushModelMatrix
{
	float4x4 model;
} push_constants;


void main()
{
	gl_Position = camera.viewProj * push_constants.model * position;
	outUV = uv;
	outMatreialID = material;
}