#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "UniformBuffers.glsl"

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 normals;
layout(location = 3) in uint material;


layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormals;
layout(location = 3) out uint outMaterialID;
layout(location = 4) out vec2 outVelocity;
layout(location = 5) out uint outFlags;

out gl_PerVertex {
    vec4 gl_Position;
};


layout(push_constant) uniform pushModelMatrix
{
	mat4 mesh;
	mat4 previousMesh;
	uint meshFlags;
} push_constants;


layout(set = 0, binding = 0) uniform UniformBufferObject 
{    
    CameraBuffer camera;
}; 


void main()
{
	const mat4 transFormation = camera.viewProj * push_constants.mesh;
	vec4 transformedPosition = transFormation * position;

	gl_Position = transformedPosition;
	outPosition = push_constants.mesh * position;
	outUV = uv;
	outNormals = mat3(push_constants.mesh) * vec3(normals.xyz);
	outMaterialID =  material;
	outFlags = push_constants.meshFlags;

	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	vec4 previousPosition = camera.previousFrameViewProj * push_constants.previousMesh * position;
	previousPosition /= previousPosition.w;
	outVelocity = previousPosition.xy - transformedPosition.xy;
}
