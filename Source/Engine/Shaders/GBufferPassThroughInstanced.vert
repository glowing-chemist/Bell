#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "MeshAttributes.glsl"
#include "UniformBuffers.glsl"

layout(location = 0) in float4 position;
layout(location = 1) in float2 uv;
layout(location = 2) in float4 normals;
layout(location = 3) in uint material;
layout(location = 3) in uint material;



layout(location = 0) out float4 outPosition;
layout(location = 1) out float2 outUV;
layout(location = 2) out float3 outNormals;
layout(location = 3) out uint outMaterialID;
layout(location = 4) out float2 outVelocity;


out gl_PerVertex {
    float4 gl_Position;
};


layout(set = 0, binding = 0) uniform UniformBufferObject 
{    
    CameraBuffer camera;
}; 

layout(set = 0, binding = 2) uniform Uniformtransformation
{    
    MeshEntry meshEntry;
} instanceTransformations[];


void main()
{
	const float4x4 transFormation = camera.viewProj * instanceTransformations[gl_InstanceID].meshEntry.mTransformation;
	float4 transformedPosition = transFormation * position;

	gl_Position = transFormation * position;
	outPosition = instanceTransformations[gl_InstanceID].meshEntry.mTransformation * position;
	outUV = uv;
	outNormals = float3x3(instanceTransformations[gl_InstanceID].meshEntry.mTransformation) * float3(normals.xyz);
	outMaterialID =  material;

	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	float4 previousPosition = camera.previousFrameViewProj * instanceTransformations[gl_InstanceID].meshEntry.mPreviousTransformation * position;
	previousPosition /= previousPosition.w;
	outVelocity = previousPosition.xy - transformedPosition.xy;
}