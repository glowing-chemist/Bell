#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "MeshAttributes.glsl"
#include "UniformBuffers.glsl"

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 normals;
layout(location = 3) in uint material;
layout(location = 3) in uint material;



layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormals;
layout(location = 3) out uint outMaterialID;
layout(location = 4) out vec2 outVelocity;


out gl_PerVertex {
    vec4 gl_Position;
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
	const mat4 transFormation = camera.viewProj * instanceTransformations[gl_InstanceID].meshEntry.mTransformation;
	vec4 transformedPosition = transFormation * position;

	gl_Position = transFormation * position;
	outPosition = instanceTransformations[gl_InstanceID].meshEntry.mTransformation * position;
	outUV = uv;
	outNormals = mat3(instanceTransformations[gl_InstanceID].meshEntry.mTransformation) * vec3(normals.xyz);
	outMaterialID =  material;

	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	vec4 previousPosition = camera.previousFrameViewProj * instanceTransformations[gl_InstanceID].meshEntry.mPreviousTransformation * position;
	previousPosition /= previousPosition.w;
	outVelocity = previousPosition.xy - transformedPosition.xy;
}