#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "UniformBuffers.glsl"


layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 normals;
layout(location = 3) in uint material;


layout(location = 0) out vec4 outNormals;
layout(location = 1) out uint outMaterialID;
layout(location = 2) out vec2 outUv;


layout(push_constant) uniform pushModelMatrix
{
	mat4 model;
} push_constants;


out gl_PerVertex {
    vec4 gl_Position;
};


layout(binding = 0) uniform UniformBufferObject {    
    CameraBuffer camera;    
}; 


void main()
{
	const mat4 transFormation = camera.viewProj * push_constants.model;

	gl_Position = transFormation * position;
	outNormals = mat3(push_constants.model) * vec3(normals.xyz);
	outMaterialID = material;
	outUv = uv;
}
