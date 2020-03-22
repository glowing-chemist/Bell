#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "UniformBuffers.glsl"


layout(location = 0) in float4 position;
layout(location = 1) in float2 uv;
layout(location = 2) in float4 normals;
layout(location = 3) in uint material;


layout(location = 0) out float4 outNormals;
layout(location = 1) out uint outMaterialID;
layout(location = 2) out float2 outUv;
layout(location = 3) out float2 outVelocity;


layout(push_constant) uniform pushModelMatrix
{
	float4x4 mesh;
	float4x4 previousMesh;
} push_constants;


out gl_PerVertex {
    float4 gl_Position;
};


layout(binding = 0) uniform UniformBufferObject {    
    CameraBuffer camera;    
}; 


void main()
{
	const float4x4 transFormation = camera.viewProj * push_constants.mesh;
	float4 transformedPosition = transFormation * position;

	gl_Position = transformedPosition;
	outNormals = float3x3(push_constants.mesh) * float3(normals.xyz);
	outMaterialID = material;
	outUv = uv;

	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	float4 previousPosition = camera.previousFrameViewProj * push_constants.previousMesh * position;
	previousPosition /= previousPosition.w;
	outVelocity = previousPosition.xy - transformedPosition.xy;
}
